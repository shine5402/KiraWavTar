#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QCoreApplication>
#include <QMenu>
#include <QObject>
#include <QVersionNumber>

class QNetworkAccessManager;

namespace UpdateChecker {
Q_NAMESPACE

class Checker : public QObject
{
    Q_OBJECT
public:
    explicit Checker(QObject *parent = nullptr) : QObject(parent) {}
    virtual void triggerUpdateCheck(QVersionNumber current = {}) = 0;

signals:
    void updateAvailable(const QVersionNumber &newVersion, const QString &msgBody, const QUrl &updateUrl);
    void alreadyUpToDate();
    void errorOccurred(const QString &msg);
};

class GithubReleaseChecker : public Checker
{
    Q_OBJECT
public:
    GithubReleaseChecker(QString owner, QString repo, QObject *parent = nullptr)
        : Checker(parent), m_owner(std::move(owner)), m_repo(std::move(repo)) {};
    virtual void triggerUpdateCheck(QVersionNumber current = {});

private:
    QString m_owner;
    QString m_repo;
    static QNetworkAccessManager *s_networkMgr;
};

enum Schedule { EVERYRUN = 0, DAILY, WEEKLY, MONTHLY, DISABLED };
Q_ENUM_NS(Schedule)

Schedule getSchedule();
void setSchedule(Schedule value);
QMenu *createMenuForSchedule();

void checkManually(Checker *checker);
void triggerScheduledCheck(Checker *checker);
}; // namespace UpdateChecker

#endif // UPDATECHECKER_H
