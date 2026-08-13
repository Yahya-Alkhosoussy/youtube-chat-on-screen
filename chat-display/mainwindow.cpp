#include "mainwindow.h"
#include "preferences.h"
#include "ui_mainwindow.h"
#include <QLabel>
#include <QTimer>
#include <QScreen>
#include <QScrollBar>
#include <string>

#define DEBUG true

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
        dialog.exec();
    });

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
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::addMessage(const QString &text) {
    auto *label = new QLabel(text, ui->layout_messages);
    label->setWordWrap(true);
    label->setStyleSheet(
        "background: rgba(255, 255, 255, 30); color: white; "
        "padding: 8px; border-radius: 6px; margin: 2px;"
    );

    // insert before the spacer
    ui->verticalLayout_messages->insertWidget(
        ui->verticalLayout_messages->count() - 1, label
    );

    QTimer::singleShot(0, this, [this]() {
        auto *bar = ui->scrollArea_messages->verticalScrollBar();
        bar->setValue(bar->maximum());
    });
}