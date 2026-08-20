#pragma once

#include <QObject>
#include <optional>
#include <QThread>
#include <QTimer>
#include "ChatResponse.h"

class ChatWorker : public QObject 
{

    Q_OBJECT

public:
    explicit ChatWorker(const QString &liveChatId, QObject *parent = nullptr);

public slots:
    void poll();
    void stopPoll();
signals:

    void messagesFetched(ChatResponse response);
    void errorOccurred(QString message);

private:
    QString liveChatid;
    QString nextPageToken;
    QTimer* pollTimer = nullptr;
};