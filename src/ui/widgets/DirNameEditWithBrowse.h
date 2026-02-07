#ifndef DIRNAMEEDITWITHBROWSE_H
#define DIRNAMEEDITWITHBROWSE_H

#include <QFileDialog>
#include <QWidget>

class QLineEdit;
class QPushButton;

class DirNameEditWithBrowse : public QWidget
{
    Q_OBJECT
public:
    explicit DirNameEditWithBrowse(QWidget *parent = nullptr);

    QString dirName() const;
    void setDirName(const QString &value);

    QString getCaption() const;
    void setCaption(const QString &value);

    QFileDialog::Options getOptions() const;
    void setOptions(const QFileDialog::Options &value);

signals:
    void browseTriggered();
    void dropTriggered();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onBrowse();

private:
    QLineEdit *m_dirNameEdit;
    QString m_caption{};
    QFileDialog::Options m_options = QFileDialog::ShowDirsOnly;
    QPushButton *m_browseButton;
};

#endif // DIRNAMEEDITWITHBROWSE_H
