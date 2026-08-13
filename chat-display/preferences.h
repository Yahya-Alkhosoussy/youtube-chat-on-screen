#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QDialog>
#include <QColor>

namespace Ui {
class preferences;
}

class preferences : public QDialog
{
    Q_OBJECT

public:
    explicit preferences(QWidget *parent = nullptr);
    ~preferences();

    bool toggleTransparency();
    QColor changeTextColour();
    int changeFontSize();

private:
    Ui::preferences *ui;
};

#endif // PREFERENCES_H
