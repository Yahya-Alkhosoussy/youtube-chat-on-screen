#include "preferences.h"
#include "ui_preferences.h"
#include <QSettings>
#include <QFont>

preferences::preferences(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::preferences)
{
    ui->setupUi(this);
    QSettings settings;
    ui->FontSizeSpinBox->setValue(settings.value("preferences/FontSize", 13).toInt());
    ui->TransparentCheckBox->setChecked(settings.value("preferences/Transparency").toBool());
    ui->colourComboBox->setCurrentText(settings.value("preference/TextColour", "white").toString());
}

int preferences::changeFontSize() {
    return ui->FontSizeSpinBox->value();
}

QColor preferences::changeTextColour() {
    return QColor(ui->colourComboBox->currentText());
}

bool preferences::toggleTransparency() {
    return ui->TransparentCheckBox->isChecked();
}

preferences::~preferences()
{
    delete ui;
}
