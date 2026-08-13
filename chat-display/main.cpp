#include <pybind11/embed.h> // slots error if it is after anything QT
#include "mainwindow.h"

#include <QApplication>
#include <QDebug>

namespace py = pybind11;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setOrganizationName("spiderbyte");
    QApplication::setApplicationName("chat-display");
    MainWindow w;

    // python stuff
    py::scoped_interpreter guard {};
    
    try {
        // Add the current directory to Python's path so it can find your script
        py::module_ sys = py::module_::import("sys");
        sys.attr("path").attr("append")("/Users/yahyaamr/Documents/GitHub/youtube-chat-on-screen/chat-display/python");

        py::module_ yt_api = py::module_::import("yt_api");
        py::object result = yt_api.attr("catch_data")();

        std::string cpp_result = result.cast<std::string>();

        qDebug() << "Test from python: " << cpp_result;

    } catch (py::error_already_set& e) {
        qDebug() << "Got an error from python! Error: " << e.what();
    }
    w.show();
    return QApplication::exec();
}
