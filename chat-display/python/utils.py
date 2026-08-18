from enum import StrEnum

from pydantic import BaseModel


class YoutubeLiveChatTextMessageDetails(BaseModel):
    messageText: str

class YoutubeLiveChatBannedUserDetails(BaseModel):
    channelId: str
    channelUrl: str
    displayName: str
    profileImageUrl: str


class YoutubeLiveChatUserBannedDetails(BaseModel):
    bannedUserDetails: YoutubeLiveChatBannedUserDetails
    banType: str # permanent or temporary only valid values
    banDurationSeconds: float

class YoutubeLiveChatMemberMilestoneDetails(BaseModel):
    userComment: str
    memberMonth: int
    memberLevelName: str

class YoutubeLiveChatNewSponsorDetails(BaseModel):
    memberLevelName: str
    isUpgrade: bool

class YoutubeLiveChatSuperChatDetails(BaseModel):
    amountMicros: float
    currency: str
    amountDisplayString: str
    userComment: str
    tier: int

class YoutubeLiveChatSuperStickerMetaData(BaseModel):
    stickerId: str
    altText: str
    language: str
    amountMicros: float
    currency: str
    amountDisplayString: str
    tier: int

class YoutubeLiveChatSuperStickerDetails(BaseModel):
    superStickerMetadata: YoutubeLiveChatSuperStickerMetaData

class YoutubeLiveChatPollOptions(BaseModel):
    optionText: str
    tally: str

class StatusStrEnum(StrEnum):
    UNKNOWN = "unknown"
    ACTIVE = "active"
    CLOSED = "closed"

class YoutubeLiveChatPollMetaData(BaseModel):
    options: YoutubeLiveChatPollOptions
    questionText: str
    status: StatusStrEnum

class YoutubeLiveChatPollDetails(BaseModel):
    metadata: YoutubeLiveChatPollMetaData

class YoutubeLiveChatMembershipGiftingDetails(BaseModel):
    giftMembershipCount: int
    giftMembershipLevelName: str

class YoutubeLiveChatGiftMembershipReceivedDetails(BaseModel):
    memberLevelName: str
    gifterChannelId: str
    associatedMembershipGiftingMessageId: str

class YoutubeLiveChatGiftDuration(BaseModel):
    seconds: int
    nanos: int

class YoutubeLiveChatGiftMetaData(BaseModel):
    jewelsAmount: int
    giftName: str
    giftUrl: str
    giftDuration: YoutubeLiveChatGiftDuration
    hasVisualEffect: bool
    comboCount: int
    altText: str
    language: str

class YoutubeLiveChatGiftEventDetails(BaseModel):
    giftMetaData: YoutubeLiveChatGiftMetaData

class YoutubeLiveChatAuthorDetails(BaseModel):
    channelId: str
    channelUrl: str
    displayName: str
    profileImageUrl: str
    isVerified: bool
    isChatOwner: bool
    isChatSponsor: bool
    isChatModerator: bool

class YoutubeLiveChatMessageSnippet(BaseModel):
    type: str
    liveChatId: str
    authorChannelId: str
    publishedAt: str
    hasDisplayContent: bool | None = None
    displayMessage: str | None = None

    # All of the following are optional fields because only one of them are returned at a time
    textMessageDetails: YoutubeLiveChatTextMessageDetails | None = None
    userBannedDetails: YoutubeLiveChatUserBannedDetails | None = None
    memberMilestoneChatDetails: YoutubeLiveChatMemberMilestoneDetails | None = None
    newSponsorDetails: YoutubeLiveChatNewSponsorDetails | None = None
    superChatDetails: YoutubeLiveChatSuperChatDetails | None = None
    superStickerDetails: YoutubeLiveChatSuperStickerDetails | None = None
    pollDetails: YoutubeLiveChatPollDetails | None = None
    membershipGiftingDetails: YoutubeLiveChatMembershipGiftingDetails | None = None
    giftMembershipReceivedDetails: YoutubeLiveChatGiftMembershipReceivedDetails | None = None
    giftEventDetails: YoutubeLiveChatGiftEventDetails | None = None

class YoutubeLiveChatMessage(BaseModel):
    kind: str | None = None
    etag: str | None = None
    id: str
    snippet: YoutubeLiveChatMessageSnippet
    authorDetails: YoutubeLiveChatAuthorDetails | None = None

class YoutubeLiveChatResponse(BaseModel):
    kind: str | None = None
    etag: str | None = None
    nextPageToken: str | None = None
    pollingIntervalMillis: int | None = 5000
    items: list[YoutubeLiveChatMessage] = []
