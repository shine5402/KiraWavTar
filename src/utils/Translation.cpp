#include "Translation.h"

#include <QApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

Translation::Translation(QLocale locale, QStringList translationFilenames, QString author)
    : m_locale(locale), m_translationFilenames(translationFilenames), m_author(author)
{
}

Translation::Translation()
{
}

QList<QTranslator *> Translation::s_installedTranslators;
Translation Translation::s_currentInstalled;

QJsonObject Translation::toJson() const
{
    QJsonObject root;
    root.insert("locale", m_locale.bcp47Name());
    root.insert("translationFilenames", QJsonArray::fromStringList(m_translationFilenames));
    root.insert("author", m_author);
    return root;
}

Translation Translation::fromJson(const QJsonObject &json)
{
    if (json.empty() || !json.contains("locale") || !json.contains("translationFilenames") || !json.contains("author"))
        return {};

    auto locale = QLocale(json.value("locale").toString());
    QStringList translationFilenames;
    for (const auto &value : json.value("translationFilenames").toArray())
        translationFilenames.append(value.toString());
    auto author = json.value("author").toString();

    return Translation(locale, translationFilenames, author);
}

void Translation::install() const
{
    uninstall();
    if (!isValid())
        return;

    for (const auto &fileName : std::as_const(m_translationFilenames)) {
        auto translator = new QTranslator(qApp);
        if (translator->load(fileName)) {
            qApp->installTranslator(translator);
            s_installedTranslators.append(translator);
            s_currentInstalled = *this;
        } else {
            qDebug() << "Failed to load translation file:" << fileName;
            delete translator;
        }
    }
}

void Translation::uninstall()
{
    s_currentInstalled = {};
    for (auto translator : std::as_const(s_installedTranslators)) {
        qApp->removeTranslator(translator);
    }
    s_installedTranslators.clear();
}

QLocale Translation::locale() const
{
    return m_locale;
}

void Translation::setLocale(const QLocale &value)
{
    m_locale = value;
}

QStringList Translation::translationFilenames() const
{
    return m_translationFilenames;
}

void Translation::setTranslationFilenames(const QStringList &value)
{
    m_translationFilenames = value;
}

QString Translation::author() const
{
    return m_author;
}

void Translation::setAuthor(const QString &value)
{
    m_author = value;
}

Translation Translation::getCurrentInstalled()
{
    return s_currentInstalled;
}

bool Translation::isValid() const
{
    return !(m_translationFilenames.isEmpty() || m_author.isEmpty());
}

bool Translation::operator==(const Translation &other) const
{
    if (!isValid() && !other.isValid())
        return true;
    return m_locale == other.m_locale && m_translationFilenames == other.m_translationFilenames &&
           m_author == other.m_author;
}
