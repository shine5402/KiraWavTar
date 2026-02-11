#ifndef ENGINESETTINGSDIALOG_H
#define ENGINESETTINGSDIALOG_H

#include <QDialog>

class QComboBox;
class QSpinBox;
class QSlider;
class QCheckBox;

class EngineSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EngineSettingsDialog(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

private:
    void accept() override;

private:
    QComboBox *m_srcQualityCombo;
    QSpinBox *m_concurrencySpinBox;
    QSlider *m_concurrencySlider;
    QCheckBox *m_autoCheckBox;
};

#endif // ENGINESETTINGSDIALOG_H
