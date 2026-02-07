#include "TranslationManager.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QAction>
#include <QObject>
#include <QSettings>
#include <algorithm>

TranslationManager* TranslationManager::instance = nullptr;

TranslationManager* TranslationManager::getManager()
{
    if (!instance)
        instance = new TranslationManager;
    return instance;
}

TranslationManager::TranslationManager()
{
    auto metaFilePath = QDir{qApp->applicationDirPath()}.filePath("translations/translations.json");
    auto metaFile = QFile{metaFilePath};
    if (!metaFile.open(QFile::Text | QFile::ReadOnly))
        return;
    auto metaFileContent = metaFile.readAll();
    auto array = QJsonDocument::fromJson(metaFileContent).array();

    for (const auto& value : array) {
        auto tr = Translation::fromJson(value.toObject());
        QStringList fullPaths;
        for (const auto& fileName : tr.translationFilenames())
            fullPaths.append(QDir{qApp->applicationDirPath()}.filePath("translations/" + fileName));
        tr.setTranslationFilenames(fullPaths);
        if (tr.isValid())
            translations.append(tr);
    }

    getTranslationFor(getLocaleUserSetting()).install();
}

void TranslationManager::setLangActionChecked(QMenu* i18nMenu, const Translation& translation) const
{
    auto actions = i18nMenu->actions();
    for (auto action : std::as_const(actions)){
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

QVector<Translation> TranslationManager::getTranslations() const
{
    return translations;
}

Translation TranslationManager::getTranslation(int i) const
{
    if (i == -1)
        return {};
    return translations.at(i);
}


Translation TranslationManager::getTranslationFor(const QLocale& locale) const
{
    auto it = std::ranges::find_if(translations, [&locale](const Translation& t) {
        return t.locale() == locale;
    });
    return it != translations.end() ? *it : Translation{};
}

int TranslationManager::getCurrentInstalledTranslationID() const
{
    auto it = std::ranges::find_if(translations, [](const Translation& elem) {
        return elem == Translation::getCurrentInstalled();
    });
    return it != translations.end() ? static_cast<int>(std::distance(translations.begin(), it)) : -1;
}

Translation TranslationManager::getCurrentInstalled() const
{
    return Translation::getCurrentInstalled();
}

QMenu* TranslationManager::getI18nMenu()
{
    if (i18nMenu)
        return i18nMenu;

    auto translations = TranslationManager::getManager()->getTranslations();
    i18nMenu = new QMenu("Language");
    auto defaultLang = new QAction("English, built-in", i18nMenu);
    defaultLang->setData(-1);
    defaultLang->setCheckable(true);
    i18nMenu->addAction(defaultLang);

    for (auto i = 0; i < translations.count(); ++i)
    {
        auto l = translations.at(i);
        auto langAction = new QAction(QLatin1String("%1 (%2), by %3").arg(QLocale::languageToString(l.locale().language()),l.locale().bcp47Name(),l.author()), i18nMenu);
        langAction->setData(i);
        langAction->setCheckable(true);
        i18nMenu->addAction(langAction);
    }
    QObject::connect(i18nMenu, &QMenu::triggered, i18nMenu, [this](QAction* action){
        auto translation = getTranslation(action->data().toInt());
        translation.install();
        setLangActionChecked(i18nMenu, translation);
        saveUserLocaleSetting(translation.locale());
    });

    setLangActionChecked(i18nMenu, getCurrentInstalled());
    return i18nMenu;
}
