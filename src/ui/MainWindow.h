#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

namespace UpdateChecker {
class GithubReleaseChecker;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void changeEvent(QEvent *event) override;

private:
    QMenu *createHelpMenu();

private slots:
    void reset();
    void updateStackWidgetIndex();
    void run();
    void fillResultPath();
    void about();

private:
    Ui::MainWindow *ui;
    UpdateChecker::GithubReleaseChecker *m_updateChecker;
};
#endif // MAINWINDOW_H
