#pragma once

#include <QString>
#include <QList>
#include <optional>

struct ChatMessage {
    QString author;
    QString message;
    QString id;
};

struct ChatResponse {
    int pollingIntervalMillis;
    QString nextPageToken;
    QList<ChatMessage> messages;
};