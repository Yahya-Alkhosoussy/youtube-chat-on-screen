#include <pybind11/embed.h>
#include "ChatWorker.h"

namespace py = pybind11;

namespace {

    static ChatMessage parseChatMessage(const py::dict& d) {
        ChatMessage msg;
        msg.author = QString::fromStdString(d["author"].cast<std::string>());
        msg.message = QString::fromStdString(d["message"].cast<std::string>());
        msg.id = QString::fromStdString(d["id"].cast<std::string>());
        return msg;
    }

    static ChatResponse parseChatResponse(const py::dict& d) {
        ChatResponse resp;
        resp.pollingIntervalMillis = d["pollingIntervalMillis"].cast<int>();
        py::object tokenObj = d["nextPageToken"];
        if (!tokenObj.is_none()) resp.nextPageToken = QString::fromStdString(tokenObj.cast<std::string>());

        for (const py::handle& item : d["messages"].cast<py::list>()) {
            resp.messages.append(parseChatMessage(item.cast<py::dict>()));
        }
        return resp;
    }
}

ChatWorker::ChatWorker(const QString& _liveChatId, QObject *parent) :
QObject(parent)
{
    liveChatid = _liveChatId;
}

void ChatWorker::startPoll() {
    pollTimer = new QTimer(this);
    connect(pollTimer, &QTimer::timeout, this, &ChatWorker::poll);
    pollTimer->start(1100);
    poll();
}

void ChatWorker::stopPoll() {
    if (pollTimer) pollTimer->stop();
}

void ChatWorker::poll() {
    try {
        py::gil_scoped_acquire aquire; // to use python from outside of the main thread


        // Add the current directory to Python's path so it can find your script
        py::module_ sys = py::module_::import("sys");
        sys.attr("path").attr("append")("/Users/yahyaamr/Documents/GitHub/youtube-chat-on-screen/chat-display/python");

        py::module_ yt_api = py::module_::import("yt_api");

        std::string live_chat_id = yt_api.attr("get_data")().cast<std::string>();

        py::object messages;

        if (!nextPageToken.isNull()) {
            messages = yt_api.attr("fetch_chat_msg")(live_chat_id, nextPageToken);
        } else {
            messages = yt_api.attr("fetch_chat_msg")(live_chat_id);
        }

        ChatResponse response = parseChatResponse(messages.cast<py::dict>());

        if (!response.nextPageToken.isNull()) {
            nextPageToken = response.nextPageToken;
        }

        if (pollTimer) {
            pollTimer->setInterval(response.pollingIntervalMillis);
        }

        emit messagesFetched(response);

    } catch (const py::error_already_set& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}