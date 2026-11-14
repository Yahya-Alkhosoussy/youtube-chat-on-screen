#include <pybind11/embed.h> // slots error if it is after anything QT
#include "mainwindow.h"
#include "updateChecker.h"
#include <iostream>

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>
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
    qDebug() << "Working directory: " << QCoreApplication::applicationDirPath();
    qDebug() << "Checking for updates";
    qDebug() << checkForNewVersion();

    bool update = checkForNewVersion();
    auto version = get_update();
    auto current = parseTags(APP_VERSION);
    if (version && current) {
        qDebug() << "Current tags: " << std::to_string(current.value()[0]) + "." + std::to_string(current.value()[1]) + "." + std::to_string(current.value()[2]);
        qDebug() << "Latest found: " << std::to_string(version->version[0]) + "." + std::to_string(version->version[1]) + "." + std::to_string(version->version[2]);
    }
    else if (!version && current) {
        qDebug() << "Could not find the tags for the latest update";
    }
    else if (!current && version) {
        qDebug() << "Could not find the tags for the current update";
    }
    else if (!current && !version) {
        qDebug() << "Coult not find the tags for both updates.";
    }

    if (update && version && current) {
        auto reply = QMessageBox::question(nullptr, "Update available",
            QString("Version %1 is available (You have %2). Download and install now?")
            .arg(QString::fromStdString(std::to_string(version->version[0]) + "." + std::to_string(version->version[1]) + "." + std::to_string(version->version[2])),
                 QString::fromStdString(std::to_string(current.value()[0]) + "." + std::to_string(current.value()[1]) + "." + std::to_string(current.value()[2])))
        );

        if (reply == QMessageBox::Yes) {
            QString installer = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                .filePath("chat-display-update.exe");
            if (updateApp(version->downloadUrl, installer)) {
                QProcess::startDetached(installer, {});
                return 0; // exit so the installer can overwrite the exe
            }
            QMessageBox::warning(nullptr, "Update failed", "Could not download the update.");
        }
    }

    #ifdef Q_OS_MAC
        QString resourcesDir = QDir(appDir).filePath("../Resources");
        QString pythonRoot = QDir(resourcesDir).filePath("python");
    #elif Q_OS_LINUX
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
            sitePackages = QDir(appDir).filePath("../../../../.venv/Lib/site-packages");
            scriptsDir = QDir(appDir).filePath("../../../../chat-display/python");
            qputenv("PYTHONHOME", findPythonHome().toUtf8().constData());
            qDebug() << "DEBUG MODE: PATHS OVERWRITTEN";
            qDebug() << "found site packages: " << sitePackages.toStdString();
            qDebug() << "found scripts: " << scriptsDir.toStdString();
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

        qDebug() << "What sys sees: " << QString::fromStdString(py::str(sys.attr("path")).cast<std::string>());

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

    //MainWindow w;
    //w.show();
    //#ifdef Q_OS_WIN
    //    w.applyClickThroughNative(false);
    //#endif
    py::gil_scoped_release release;

    checkForNewVersion();

    return QApplication::exec();
}
