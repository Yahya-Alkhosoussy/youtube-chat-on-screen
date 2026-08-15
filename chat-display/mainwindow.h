#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QColor>
#include <QMainWindow>
#include <QQueue>

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

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    Ui::MainWindow *ui;

    void addMessage(const QString &text); // to add a message on the display
    // helpful comment right? lol
    void applySettingsChanges(int fontSize, QColor colour, bool transparent, RGBA backgroundColor);
    void start_poll_timer();
    int poll_for_messages();
};
#endif // MAINWINDOW_H
