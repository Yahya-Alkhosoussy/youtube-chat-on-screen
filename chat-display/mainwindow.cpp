#include <pybind11/embed.h>

#include "ChatResponse.h"
#include "mainwindow.h"
#include "preferences.h"
#include "ui_mainwindow.h"
#include <QLabel>
#include <QTimer>
#include <QScreen>
#include <QScrollBar>
#include <QSettings>
#include <QMessageBox>
#include <QMediaFormat>
#include <string>

#define DEBUG false

namespace py = pybind11;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);

    connect(ui->actionExit, &QAction::triggered, this, &QMainWindow::close);
    connect(ui->actionPrefrences, &QAction::triggered, this, [this]() {
        preferences dialog(this);
        if (dialog.exec() == QDialog::Accepted) {
            int size = dialog.getFontSize();
            QColor color = dialog.getTextColour();
            QString color_name = dialog.getColourName();
            bool transparent = dialog.getTransparency();
            int rValue = dialog.getRValue();
            int gValue = dialog.getGValue();
            int bValue = dialog.getBValue();
            int aValue = dialog.getAValue();
            RGBA backgroundColour(rValue, gValue, bValue, aValue);

            applySettingsChanges(size, color, transparent, backgroundColour);
            QSettings settings;
            qDebug() << "Color name: " << color_name;
            settings.setValue("preferences/fontSize", size);
            settings.setValue("preferences/textColor", color_name);
            settings.setValue("preferences/transparentBackground", transparent);
            settings.setValue("preferences/rValue", rValue);
            settings.setValue("preferences/gValue", gValue);
            settings.setValue("preferences/bValue", bValue);
            settings.setValue("preferences/aValue", aValue);
        }
    });

    QSettings settings;
    int size = settings.value("preferences/fontSize").toInt();
    QColor color = settings.value("preferences/textColor").toString();
    bool transparent = settings.value("preferences/transparentBackground").toBool();
    RGBA backgroundColor(settings.value("preferences/rValue").toInt(), settings.value("preferences/gValue").toInt(), 
    settings.value("preferences/bValue").toInt(), settings.value("preferences/aValue").toInt());

    applySettingsChanges(size, color, transparent, backgroundColor);

    m_audioOutput = new QAudioOutput(this);
    m_notificationPlayer = new QMediaPlayer(this);
    m_notificationPlayer->setAudioOutput(m_audioOutput);
    m_notificationPlayer->setSource(QUrl("qrc:/sounds/universfield-message-ping-351298.wav"));
    m_audioOutput->setVolume(0.5); // 0.0–1.0

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect available = screen->availableGeometry();
    resize(width(), available.height());
    move(x(), available.top());

    if (DEBUG)
    { 
        addMessage("First message"); 

        for (int i = 0; i < 25; i++) {
            QTimer::singleShot(250, this, [this]() {
                addMessage("message");
            });
        }
    }

    try {
        py::module_ yt_api = py::module_::import("yt_api");

        std::string live_chat_id = yt_api.attr("get_data")().cast<std::string>();


        qRegisterMetaType<ChatResponse>("ChatResponse");

        worker = new ChatWorker(QString::fromStdString(live_chat_id));
        worker->moveToThread(&worker_thread);

        connect(worker, &ChatWorker::messagesFetched, this, &MainWindow::onMessageFetched);
        connect(worker, &ChatWorker::errorOccurred, this, &MainWindow::onErrorOccurred);
        connect(&worker_thread, &QThread::finished, worker, &QObject::deleteLater);

        worker_thread.start();
    } catch (const py::error_already_set &e) {
    QMessageBox::critical(this, "Startup Error",
        QString("Failed to connect to YouTube chat:\n%1").arg(e.what()));
    }
}

MainWindow::~MainWindow()
{
    worker_thread.quit();
    worker_thread.wait();
    delete ui;
}


void MainWindow::addMessage(const QString &text) {
    auto *label = new QLabel(text, ui->layout_messages);
    label->setWordWrap(true);
    label->setMaximumWidth(420);
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QString style = QString("background: rgba(0, 0, 0, 80); padding: 8px; border-radius: 6px; margin: 2px; color: %1;").arg(current_color.name());
    label->setStyleSheet(style);
    label->setFont(current_font);

    // insert before the spacer
    ui->verticalLayout_messages->insertWidget(
        ui->verticalLayout_messages->count() - 1, label
    );

    QTimer::singleShot(0, this, [this]() {
        auto *bar = ui->scrollArea_messages->verticalScrollBar();
        bar->setValue(bar->maximum());
    });
}

void MainWindow::applySettingsChanges(int fontSize, QColor colour, bool transparent, RGBA backgroundColor) {
    QFont font;
    font.setPointSize(fontSize);

    QString background = QString("background: rgba(%1 ").arg(backgroundColor.rValue) + QString(", %1,").arg(backgroundColor.gValue)
    + QString("%1, ").arg(backgroundColor.bValue) + QString("%1); ").arg(backgroundColor.aValue);

    QString colourStyle = background + QString("padding: 8px; border-radius: 6px; margin: 2px; color: %1;").arg(colour.name());
    
    current_color = colour;
    current_font = font;
    // Apply to every existing message widget
    for (int i = 0; i < ui->verticalLayout_messages->count(); ++i) {
        QWidget *widget = ui->verticalLayout_messages->itemAt(i)->widget();
        if (widget) {
            widget->setFont(font);
            widget->setStyleSheet(colourStyle);
        }
    }

    // Toggle transparency
    if (transparent) {
        centralWidget()->setStyleSheet("background: transparent;");
    } else {
        centralWidget()->setStyleSheet("background: rgb(30, 30, 30);");
    }
}

void MainWindow::onMessageFetched(ChatResponse response) {
    bool newMessage = false;
    for (auto message : response.messages) {
        if (seen_messages.contains(message.id)) continue; // the message was seen, do not add it.

        newMessage = true;
        seen_messages.insert(message.id);
        messageIdOrder.enqueue(message.id);
        if (messageIdOrder.size() > MAX_IDS_STORED) seen_messages.remove(messageIdOrder.dequeue()); // forcefully removes the first message from the queue.

        addMessage(QString(message.author) + QString(": ") + QString(message.message));
    }
    if (newMessage) {
        m_notificationPlayer->setPosition(0); // rewind, in case it's still finishing from a rapid prior message
        m_notificationPlayer->play();
    }
}

void MainWindow::onErrorOccurred(QString error) {
    qWarning() << "Chat polling error: " << error;
}

void MainWindow::changeEvent(QEvent* event) {
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::ActivationChange) {
        setAttribute(Qt::WA_TransparentForMouseEvents, !isActiveWindow());
    }
}