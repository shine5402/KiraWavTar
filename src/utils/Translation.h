#ifndef TRANSLATION_H
#define TRANSLATION_H

#include <QLocale>
#include <QObject>
#include <QTranslator>

class Translation
{
public:
    Translation(QLocale locale, QStringList translationFilenames, QString author);
    Translation();

    QJsonObject toJson() const;
    static Translation fromJson(const QJsonObject &jsonDoc);

    void install() const;
    static void uninstall();

    QLocale locale() const;
    void setLocale(const QLocale &value);

    QStringList translationFilenames() const;
    void setTranslationFilenames(const QStringList &value);

    QString author() const;
    void setAuthor(const QString &value);

    static Translation getCurrentInstalled();

    bool isValid() const;

    bool operator==(const Translation &other) const;

private:
    static QList<QTranslator *> s_installedTranslators;
    static Translation s_currentInstalled;

    QLocale m_locale = QLocale("en_US");
    QStringList m_translationFilenames;
    QString m_author;
};

#endif // TRANSLATION_H
