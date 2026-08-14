#include <pybind11/embed.h> // slots error if it is after anything QT
#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QMessageBox>

namespace py = pybind11;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setOrganizationName("spiderbyte");
    QApplication::setApplicationName("chat-display");

    // python stuff
    py::scoped_interpreter guard {};
    
    try {
        // Add the current directory to Python's path so it can find your script
        py::module_ sys = py::module_::import("sys");
        sys.attr("path").attr("append")("/Users/yahyaamr/Documents/GitHub/youtube-chat-on-screen/chat-display/python");

        py::module_ yt_api = py::module_::import("yt_api");

        bool needsAuth = true;
        try {
            needsAuth = !yt_api.attr("has_cached_credentials")().cast<bool>();
        } catch(py::error_already_set&) {
            needsAuth = true;
        }

        if (needsAuth) {
            QMessageBox::StandardButton reply = QMessageBox::question (
                nullptr,
                "Connect to YouTube",
                "This app needs to connect to your YouTube account to read live chat.\n\n"
                "Click OK to open your browser and sign in.",
                QMessageBox::Ok | QMessageBox::Cancel,
                QMessageBox::Ok
            );

            if (reply == QMessageBox::Cancel) {
                return 0;
            }

            try {
                yt_api.attr("initialize_auth")();
            } catch (const py::error_already_set &e) {
                QMessageBox::critical(nullptr, "Authentication Failed",
                                    QString("Could not authenticate with YouTube:\n%1").arg(e.what()));
                return 1;
            }
        }

        py::object result = yt_api.attr("catch_data")();

        std::string cpp_result = result.cast<std::string>();

        qDebug() << "Test from python: " << cpp_result;

    } catch (py::error_already_set& e) {
        qDebug() << "Got an error from python! Error: " << e.what();
    }
    MainWindow w;
    w.show();
    return QApplication::exec();
}
