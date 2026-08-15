#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <QString>
#include <QList>
#include <optional>

namespace py = pybind11;

struct ChatMessage {
    QString author;
    QString message;
    QString id;

    static ChatMessage FromPyDict(const py::dict& dict) {
        ChatMessage msg;
        msg.author = QString::fromStdString(dict["author"].cast<std::string>());
        msg.message = QString::fromStdString(dict["message"].cast<std::string>());
        msg.id = QString::fromStdString(dict["id"].cast<std::string>());
        return msg;
    }
};

struct ChatResponse {
    int pollingIntervalMillis;
    QString nextPageToken;
    QList<ChatMessage> messages;

    static ChatResponse FromPyDict(const py::dict& dict) {
        ChatResponse resp;
        resp.pollingIntervalMillis = dict["pollingIntervalMillis"].cast<int>();
        py::object obj = dict["nextPageToken"];
        if (!obj.is_none()) resp.nextPageToken = QString::fromStdString(obj.cast<std::string>());

        for (const py::handle& item : dict["messages"].cast<py::list>()) {
            resp.messages.append(ChatMessage::FromPyDict(item.cast<py::dict>()));
        }
        return resp;
    }
};