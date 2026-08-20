#include "preferences.h"
#include "ui_preferences.h"
#include <QSettings>
#include <QFont>
#include <QDebug>

preferences::preferences(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::preferences)
{
    ui->setupUi(this);

    QSettings settings;
    int size = settings.value("preferences/fontSize").toInt();
    QString color = settings.value("preferences/textColor", "White").toString();
    bool transparent = settings.value("preferences/transparentBackground").toBool();
    int rValue = settings.value("preferences/rValue").toInt();
    int gValue = settings.value("preferences/gValue").toInt();
    int bValue = settings.value("preferences/bValue").toInt(); 
    int aValue = settings.value("preferences/aValue").toInt();
    int audioValue = settings.value("preferences/audioValue").toInt();

    qDebug() << "Combo box text:" << ui->colourComboBox->currentText()
             << "| Resulting QColor:" << color
             << "| color.name():" << color;

    ui->FontSizeSpinBox->setValue(size);
    ui->TransparentCheckBox->setChecked(transparent);
    ui->colourComboBox->setCurrentText(color);
    ui->rSpinnerBox->setValue(rValue);
    ui->gSpinnerBox->setValue(gValue);
    ui->bSpinnerBox->setValue(bValue);
    ui->aSpinnerBox->setValue(aValue);
    ui->AudioSpinnerBox->setValue(audioValue);
}

int preferences::getFontSize() {
    return ui->FontSizeSpinBox->value();
}

QColor preferences::getTextColour() {
    return QColor(ui->colourComboBox->currentText());
}

bool preferences::getTransparency() {
    return ui->TransparentCheckBox->isChecked();
}

int preferences::getAValue() {
    return ui->aSpinnerBox->value();
}

int preferences::getBValue() {
    return ui->bSpinnerBox->value();
}

int preferences::getGValue() {
    return ui->gSpinnerBox->value();
}

int preferences::getRValue() {
    return ui->rSpinnerBox->value();
}

int preferences::getAudioValue() {
    return ui->AudioSpinnerBox->value();
}

QString preferences::getColourName() {
    return ui->colourComboBox->currentText();
}

preferences::~preferences()
{
    delete ui;
}
