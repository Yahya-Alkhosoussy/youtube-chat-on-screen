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
#include <string>

#define DEBUG true

namespace py = pybind11;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);

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

    _poll_timer = new QTimer(this);
    connect(_poll_timer, &QTimer::timeout, this, &MainWindow::poll_for_messages);
    // defer the actual start until after the window is shown and the event loop is running
    QTimer::singleShot(0, this, &MainWindow::start_poll_timer);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::addMessage(const QString &text) {
    auto *label = new QLabel(text, ui->layout_messages);
    label->setWordWrap(true);
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

void MainWindow::start_poll_timer() {
    int milliseconds = poll_for_messages(); // call initially and find out the interval youtube gives
    _poll_timer->start(milliseconds);
}

int MainWindow::poll_for_messages() {    
    try {
        // Add the current directory to Python's path so it can find your script
        py::module_ sys = py::module_::import("sys");
        sys.attr("path").attr("append")("/Users/yahyaamr/Documents/GitHub/youtube-chat-on-screen/chat-display/python");

        py::module_ yt_api = py::module_::import("yt_api");

        std::string live_chat_id = yt_api.attr("get_data")().cast<std::string>();

        py::object messages;

        if (!next_page_token.isNull()) {
            messages = yt_api.attr("fetch_chat_msg")(live_chat_id, next_page_token);
        } else {
            messages = yt_api.attr("fetch_chat_msg")(live_chat_id);
        }
        ChatResponse response = ChatResponse::FromPyDict(messages.cast<py::dict>());
    
        for (auto message : response.messages) {
            addMessage(QString(message.author) + QString(": ") + QString(message.message));
        }
        next_page_token = response.nextPageToken;
        return response.pollingIntervalMillis;
    } catch (py::error_already_set& e) {
        QMessageBox::critical(
            nullptr, 
            "Authentication Failed",
            QString("Could not authenticate with YouTube:\n%1").arg(e.what())
        );
        return 1;
    }
}