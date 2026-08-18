#include <pybind11/embed.h>
#include "ChatWorker.h"
#include <QDebug>

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

void ChatWorker::poll() {
    try {
        qDebug() << "Polling for message";

        py::gil_scoped_acquire aquire; // to use python from outside of the main thread

        py::module_ yt_api = py::module_::import("yt_api");

        yt_api.attr("stream_chat_messages")(
            liveChatid.toStdString(),
            py::cpp_function([this](py::object response) {
                ChatResponse parsed = parseChatResponse(response);
                emit messagesFetched(parsed);
            })
        );

    } catch (const py::error_already_set& e) {
        qDebug() << "Got an error";
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}