#include "TranslationManager.h"

#include <QAction>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QSettings>
#include <algorithm>

TranslationManager *TranslationManager::s_instance = nullptr;

TranslationManager *TranslationManager::getManager()
{
    if (!s_instance)
        s_instance = new TranslationManager;
    return s_instance;
}

TranslationManager::TranslationManager()
{
    auto metaFilePath = QDir{qApp->applicationDirPath()}.filePath("translations/translations.json");
    auto metaFile = QFile{metaFilePath};
    if (!metaFile.open(QFile::Text | QFile::ReadOnly))
        return;
    auto metaFileContent = metaFile.readAll();
    auto array = QJsonDocument::fromJson(metaFileContent).array();

    for (const auto &value : array) {
        auto tr = Translation::fromJson(value.toObject());
        QStringList fullPaths;
        for (const auto &fileName : tr.translationFilenames())
            fullPaths.append(QDir{qApp->applicationDirPath()}.filePath("translations/" + fileName));
        tr.setTranslationFilenames(fullPaths);
        if (tr.isValid())
            m_translations.append(tr);
    }

    getTranslationFor(getLocaleUserSetting()).install();
}

void TranslationManager::setLangActionChecked(QMenu *i18nMenu, const Translation &translation) const
{
    auto actions = i18nMenu->actions();
    for (auto action : std::as_const(actions)) {
        auto currTr = TranslationManager::getManager()->getTranslation(action->data().toInt());
        action->setChecked(currTr == translation);
    }
}

void TranslationManager::saveUserLocaleSetting(QLocale locale) const
{
    QSettings settings;
    settings.setValue("locale", locale);
}

QLocale TranslationManager::getLocaleUserSetting() const
{
    QSettings settings;
    return settings.value("locale", QLocale::system()).value<QLocale>();
}

QList<Translation> TranslationManager::getTranslations() const
{
    return m_translations;
}

Translation TranslationManager::getTranslation(int i) const
{
    if (i == -1)
        return {};
    return m_translations.at(i);
}

Translation TranslationManager::getTranslationFor(const QLocale &locale) const
{
    auto it = std::ranges::find_if(m_translations, [&locale](const Translation &t) { return t.locale() == locale; });
    return it != m_translations.end() ? *it : Translation{};
}

int TranslationManager::getCurrentInstalledTranslationID() const
{
    auto it = std::ranges::find_if(m_translations,
                                   [](const Translation &elem) { return elem == Translation::getCurrentInstalled(); });
    return it != m_translations.end() ? static_cast<int>(std::distance(m_translations.begin(), it)) : -1;
}

Translation TranslationManager::getCurrentInstalled() const
{
    return Translation::getCurrentInstalled();
}

QMenu *TranslationManager::getI18nMenu()
{
    if (m_i18nMenu)
        return m_i18nMenu;

    auto translations = TranslationManager::getManager()->getTranslations();
    m_i18nMenu = new QMenu("Language");
    auto defaultLang = new QAction("English, built-in", m_i18nMenu);
    defaultLang->setData(-1);
    defaultLang->setCheckable(true);
    m_i18nMenu->addAction(defaultLang);

    for (auto i = 0; i < translations.count(); ++i) {
        auto l = translations.at(i);
        auto langAction =
            new QAction(QLatin1String("%1 (%2), by %3")
                            .arg(QLocale::languageToString(l.locale().language()), l.locale().bcp47Name(), l.author()),
                        m_i18nMenu);
        langAction->setData(i);
        langAction->setCheckable(true);
        m_i18nMenu->addAction(langAction);
    }
    QObject::connect(m_i18nMenu, &QMenu::triggered, m_i18nMenu, [this](QAction *action) {
        auto translation = getTranslation(action->data().toInt());
        translation.install();
        setLangActionChecked(m_i18nMenu, translation);
        saveUserLocaleSetting(translation.locale());
    });

    setLangActionChecked(m_i18nMenu, getCurrentInstalled());
    return m_i18nMenu;
}
