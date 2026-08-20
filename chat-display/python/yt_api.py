import json
import os
import threading
from collections.abc import Callable
from dataclasses import dataclass
from pathlib import Path

import stream_list_pb2
from google.auth.transport.requests import Request
from google.oauth2.credentials import Credentials
from google.protobuf.json_format import MessageToDict
from google_auth_oauthlib.flow import InstalledAppFlow
from googleapiclient.discovery import build
from pydantic import BaseModel
from stream_list_pb2_grpc import V3DataLiveChatMessageServiceStub, grpc
from utils import YoutubeLiveChatResponse

# Scope needed to read YouTube Live Chat data
SCOPES = ['https://www.googleapis.com/auth/youtube.readonly']

youtube = None

_active_channel = None
_channel_lock = threading.Lock()

def get_authenticated_service():
    """Handles the OAuth2 flow and caches credentials in token.json."""
    global youtube
    if youtube:
        return youtube
    creds = None

    # Absolute paths are safer when running from C++
    script_dir = Path(__file__).parent / "secrets"
    if not script_dir.exists():
        os.mkdir(script_dir)

    token_path = os.path.join(script_dir, "token.json")
    client_secret_path = os.path.join(script_dir, "client_secret.json")

    # 1. Check if we already saved a user token from a previous run
    if os.path.exists(token_path):
        creds = Credentials.from_authorized_user_file(token_path, SCOPES)

    # 2. If there are no valid credentials, handle login or token refresh
    if not creds or not creds.valid:
        if creds and creds.expired and creds.refresh_token:
            # Silently refresh the expired token in the background
            creds.refresh(Request())
        else:
            # Opens a local browser window for the user to grant consent
            flow = InstalledAppFlow.from_client_secrets_file(client_secret_path, SCOPES)
            creds = flow.run_local_server(port=0)

        # Save the credentials for next time
        with open(token_path, 'w') as token:
            token.write(creds.to_json())

    # Build and return the YouTube API client object
    youtube = build('youtube', 'v3', credentials=creds)


@dataclass(frozen=True)
class Creds(BaseModel):
    token: str
    refresh_token: str
    token_uri: str
    client_id: str
    client_secret: str
    scopes: list[str]
    universe_domain: str
    account: str
    expiry: str


def get_credentials() -> Creds:
    with open(Path(__file__).parent / "secrets" / "token.json") as f:
        return Creds.model_validate(json.load(f))

def initialize_auth():
    """Explicit startup entry point — call once from C++ before showing the main window."""
    get_authenticated_service()
    return True


def has_cached_credentials():
    """Lets C++ check if a browser prompt is actually needed before showing the dialog."""
    script_dir = Path(__file__).parent / "secrets"
    token_path = os.path.join(script_dir, "token.json")
    if not os.path.exists(token_path):
        return False
    creds = Credentials.from_authorized_user_file(token_path, SCOPES)
    return bool(creds is not None and (creds.valid or (creds.expired and creds.refresh_token)))

def get_data():
    if youtube is None:
        raise ValueError("Youtube is set to None")
    try:
        # Example: Fetch details of the active user's current live broadcast
        request = youtube.liveBroadcasts().list(
            part="snippet,contentDetails,status",
            mine=True
        )
        response = request.execute()

        # Check if there's an active broadcast with a liveChatId
        items = response.get("items", [])
        if items:
            live_chat_id = items[0]["snippet"].get("liveChatId")
            return live_chat_id
        else:
            raise ValueError("Stream not found")

    except Exception as e: #noqa
        raise ValueError(f"Coult not fetch. {e}")

def fetch_chat_msg(live_chat_id: str, next_page_token: str | None = None):
    """
    Fetches the latest messages for a given liveChatId.
    Returns a JSON string containing the new messages, nextPageToken, and polling interval.
    """
    if youtube is None:
        raise ValueError("Youtube is set to None")
    try:
        # Build the request
        request = youtube.liveChatMessages().list(
            liveChatId=live_chat_id,
            part="snippet,authorDetails",
            pageToken=next_page_token  # If None, fetches the initial batch
        )
        response = request.execute()

        parsed_response = YoutubeLiveChatResponse.model_validate(response)
        messages = parsed_response.items

        # Return structured data back to C++
        return {
                "pollingIntervalMillis": parsed_response.pollingIntervalMillis,
                "nextPageToken": parsed_response.nextPageToken,
                "messages": [
                    {
                        "id": msg.id,
                        "author": msg.authorDetails.displayName if msg.authorDetails else "Unknown",
                        "message": msg.snippet.displayMessage or ""
                    }
                    for msg in messages
                ]
            }
    except Exception as e: # noqa
        raise RuntimeError("Something went wrong")

def stream_chat_messages(live_chat_id: str, callback: Callable):
    global _active_channel

    creds = get_credentials()
    channel_creds = grpc.ssl_channel_credentials()
    with grpc.secure_channel("dns:///youtube.googleapis.com:443", channel_creds) as channel:

        with _channel_lock:
            _active_channel = channel

        stub = V3DataLiveChatMessageServiceStub(channel)
        metadata = (("authorization", "Bearer " + creds.token),)
        next_page_token = None
        while True:
            request = stream_list_pb2.LiveChatMessageListRequest(
                part=["snippet", "authorDetails"],
                live_chat_id=live_chat_id,
                page_token=next_page_token,
            )
            for response in stub.StreamList(request, metadata=metadata):
                try:
                    _response = YoutubeLiveChatResponse.model_validate(
                        MessageToDict(response, preserving_proto_field_name=False, always_print_fields_with_no_presence=True)
                    )
                except Exception as e:  # noqa: BLE001
                    print("VALIDATION FAILED:", e)  # temporary — stop silently swallowing this
                    continue
                _to_send = {
                    "pollingIntervalMillis": _response.pollingIntervalMillis,
                    "nextPageToken": _response.nextPageToken,
                    "messages": [
                        {
                            "id": message.id,
                            "author": message.authorDetails.displayName if message.authorDetails else "unknown",
                            "message": message.snippet.displayMessage or ""
                        }
                        for message in _response.items
                    ]
                }
                callback(_to_send)
                next_page_token = _response.nextPageToken

def catch_data():
    data = {"author": "User1", "message": "Hello from Python!"}
    # Print as JSON string to standard output
    return json.dumps(data)

def print_stream_url():
    if youtube is None:
        raise ValueError("Youtube is still set to none")

    request = youtube.liveBroadcasts().list(
        part="snippet",
        mine=True
    )
    response = request.execute()
    items = response.get("items", [])

    if items:
        video_id = items[0]["id"]
        print("OPEN THIS LINK IN YOUR BROWSER TO TYPE CHAT:")
        print(f"https://www.youtube.com/watch?v={video_id}")
    else:
        print("No active broadcast found.")

def cancel_stream():
    with _channel_lock:
        if _active_channel is not None:
            _active_channel.close()

if __name__ == "__main__":
    def got_msg(msg):
        print(msg)

    try:
        get_authenticated_service()
        print("Starting to listen")
        print_stream_url()
        data = stream_chat_messages(get_data(), got_msg)

    except KeyboardInterrupt:
        print("\nStopped listening.")
