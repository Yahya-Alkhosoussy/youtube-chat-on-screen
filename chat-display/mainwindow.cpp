#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QLabel>
#include <QTimer>
#include <QScrollBar>

#define DEBUG true

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect available = screen->availableGeometry();
    resize(width(), available.height());
    move(x(), available.top());

    if (DEBUG)
    { 
        addMessage("First message"); 
        QTimer::singleShot(1000, this, [this]() {
            addMessage("Second message");
        });
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