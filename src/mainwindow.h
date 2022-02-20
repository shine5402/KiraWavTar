#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

namespace UpdateChecker {
    class GithubReleaseChecker;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    UpdateChecker::GithubReleaseChecker* updateChecker;

private slots:
    void reset();
    void updateStackWidgetIndex();
    void run();
    void fillResultPath();
    void about();

protected:
    void changeEvent(QEvent* event) override;
};
#endif // MAINWINDOW_H
