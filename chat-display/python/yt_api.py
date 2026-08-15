import json
import os
from pathlib import Path

from google.auth.transport.requests import Request
from google.oauth2.credentials import Credentials
from google_auth_oauthlib.flow import InstalledAppFlow
from googleapiclient.discovery import build
from utils import YoutubeLiveChatResponse

# Scope needed to read YouTube Live Chat data
SCOPES = ['https://www.googleapis.com/auth/youtube.readonly']

youtube = None

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
                "nextPageToken": parsed_response.next_page_token,
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

if __name__ == "__main__":
    import time
    try:
        get_authenticated_service()
        print("Starting to listen")
        print_stream_url()
        next_page_token = None
        while True:
            # Call your function
            data = fetch_chat_msg(get_data(), next_page_token)

            for msg in data.get("messages", []):
                print(f"[{msg['author']}]: {msg['message']}")

            # Update token and interval
            next_page_token = data.get("nextPageToken")
            poll_interval = data.get("pollingIntervalMillis", 5000) / 1000.0
            print("sleeping: ", poll_interval)

            # Wait before next poll
            time.sleep(poll_interval)
    except KeyboardInterrupt:
        print("\nStopped listening.")