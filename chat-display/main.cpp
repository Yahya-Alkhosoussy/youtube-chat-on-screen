#include <pybind11/embed.h> // slots error if it is after anything QT
#include "mainwindow.h"
#include <iostream>

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QMessageBox>

namespace py = pybind11;

int main(int argc, char *argv[])
{
    std::cout << "Entered the main file" << std::endl;

    QApplication a(argc, argv);
    QApplication::setOrganizationName("spiderbyte");
    QApplication::setApplicationName("chat-display");
    MainWindow w;

    std::cout << "Made the application and main window" << std::endl;

    QString appDir = QCoreApplication::applicationDirPath();

    std::cout << "Found App Dir" << std::endl;

    #ifdef Q_OS_MAC
        QString resourcesDir = QDir(appDir).filePath("../Resources");
        QString pythonRoot = QDir(resourcesDir).filePath("python");
    #elif defined(Q_OS_WIN)
        QString pythonRoot = QDir(appDir).filePath("../python"); // sibling of bin/, matches your install layout
    #endif

    std::cout << "Found python root" << std::endl;

    QString sitePackages = QDir(pythonRoot).filePath("site-packages");

    std::cout << "found site packages" << std::endl;
    QString scriptsDir = QDir(pythonRoot).filePath("scripts");
    std::cout << "found scripts" << std::endl;


    #ifdef Q_OS_WIN
        qputenv("PYTHONHOME", pythonRoot.toUtf8().constData());
        std::cout << "Python home is now in env" << std::endl;
    #endif


    QByteArray pathEnv = (sitePackages + QDir::listSeparator() + scriptsDir).toUtf8();
    qputenv("PYTHONPATH", pathEnv);
    std::cout << "Python path now in env" << std::endl;

    // python stuff
    py::scoped_interpreter guard {};
    std::cout << "Started python stuff" << std::endl;
    
    try {
        // Add the current directory to Python's path so it can find your script
        py::module_ sys = py::module_::import("sys");
        qDebug() << "Embedded Python version:" << QString::fromStdString(sys.attr("version").cast<std::string>());
        qDebug() << "Embedded Python executable:" << QString::fromStdString(sys.attr("executable").cast<std::string>());
        sys.attr("path").attr("append")("/Users/yahyaamr/Documents/GitHub/youtube-chat-on-screen/chat-display/python");
        sys.attr("path").attr("append")("/Users/yahyaamr/Documents/GitHub/youtube-chat-on-screen/.venv/lib/python3.12/site-packages");

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
                    QString("Could not authenticate with YouTube:\n%1").arg(e.what())
                );
                
                return 1;
            }
        } else {
            yt_api.attr("get_authenticated_service")();
        }

    } catch (py::error_already_set& e) {
        qDebug() << "Got an error from python! Error: " << e.what();
        return 1;
    }
    w.show();
    py::gil_scoped_release release;

    return QApplication::exec();
}
