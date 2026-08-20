#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QColor>
#include <QMainWindow>
#include <QQueue>
#include <QThread>
#include <qmediaplayer.h>
#include <QAudioOutput>
#include "ChatWorker.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    QFont current_font;
    QColor current_color;
    struct RGBA {
        int rValue, gValue, bValue, aValue;
        RGBA(int r, int g, int b, int a): rValue(r), gValue(g), bValue(b), aValue(a) {};
    };
    QTimer* _poll_timer;
    QString next_page_token;
    QQueue<QString> messageIdOrder;
    QSet<QString> seen_messages;
    static constexpr int MAX_IDS_STORED = 500;
    QThread worker_thread;
    ChatWorker* worker = nullptr;
    QMediaPlayer *m_notificationPlayer = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
    QPoint dragPosition;
    QShortcut *toggleShortcut = nullptr;
    bool interactive = true;

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void playNotificationSound(int audioLevel);

    #ifdef Q_OS_WIN
        void applyClickThroughNative(bool clickThrough);
    #endif

private:
    Ui::MainWindow *ui;

    void addMessage(const QString &text); // to add a message on the display
    // helpful comment right? lol
    void applySettingsChanges(int fontSize, QColor colour, bool transparent, RGBA backgroundColor, int audioValue, QKeySequence visibilityShortcut);

private slots:
    void onMessageFetched(ChatResponse resp);
    void onErrorOccurred(QString error);
    void toggleInteractive();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
};
#endif // MAINWINDOW_H
