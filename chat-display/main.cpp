#include <pybind11/embed.h> // slots error if it is after anything QT
#include "mainwindow.h"
#include <iostream>

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QProcess>

namespace py = pybind11;

#ifdef Q_OS_WIN
    #ifdef _DEBUG
        QString findPythonHome() {
            QProcess process;
            process.start("python", QStringList() << "-c" << "import sys; print(sys.base_prefix)");
            process.waitForFinished();
            QString result = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
            std::cout << result.toStdString() << std::endl;
            return result;
        }
    #endif
#endif

int main(int argc, char *argv[])
{
    std::cout << "Entered the main file" << std::endl;

    QApplication a(argc, argv);
    QApplication::setOrganizationName("spiderbyte");
    QApplication::setApplicationName("chat-display");

    std::cout << "Made the application and main window" << std::endl;

    QString appDir = QCoreApplication::applicationDirPath();

    std::cout << "Found App Dir" << std::endl;

    #ifdef Q_OS_MAC
        QString resourcesDir = QDir(appDir).filePath("../Resources");
        QString pythonRoot = QDir(resourcesDir).filePath("python");
    #elif defined(Q_OS_WIN)
        #ifndef _DEBUG 
            QString pythonRoot = QDir(appDir).filePath("../python"); // sibling of bin/, matches your install layout
        #else
            QString pythonRoot = QDir(appDir).filePath("../venv");
        #endif
    #endif

    std::cout << "Found python root" << std::endl;

    QString sitePackages = QDir(pythonRoot).filePath("site-packages");

    std::cout << "found site packages" << std::endl;
    QString scriptsDir = QDir(pythonRoot).filePath("scripts");
    std::cout << "found scripts" << std::endl;


    #ifdef Q_OS_WIN
        #ifndef _DEBUG
            qputenv("PYTHONHOME", pythonRoot.toUtf8().constData());
        #else
            sitePackages = QDir(appDir).filePath("../../.venv/Lib/site-packages");
            scriptsDir = QDir(appDir).filePath("../../chat-display/python");
            qputenv("PYTHONHOME", findPythonHome().toUtf8().constData());
            std::cout << "DEBUG MODE: PATHS OVERWRITTEN" << std::endl;
            std::cout << "found site packages: " << sitePackages.toStdString() << std::endl;
            std::cout << "found scripts: " << scriptsDir.toStdString() << std::endl;
        #endif
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

    MainWindow w;
    w.show();
    #ifdef Q_OS_WIN
        w.applyClickThroughNative(false);
    #endif
    py::gil_scoped_release release;

    return QApplication::exec();
}
