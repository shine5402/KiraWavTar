#include "TranslationManager.h"

#include <QAction>
#include <QCoreApplication>
#include <QSettings>
#include <algorithm>

using namespace Qt::StringLiterals;

TranslationManager *TranslationManager::s_instance = nullptr;

TranslationManager *TranslationManager::getManager()
{
    if (!s_instance)
        s_instance = new TranslationManager;
    return s_instance;
}

TranslationManager::TranslationManager()
{
    // These match I18N_TRANSLATED_LANGUAGES in CMakeLists.txt
    m_supportedLocales = {QLocale(QLocale::Chinese, QLocale::SimplifiedChineseScript, QLocale::China),
                          QLocale(QLocale::Japanese, QLocale::Japan)};

    // Install user's preferred translation
    installTranslation(getLocaleUserSetting());
}

QList<QLocale> TranslationManager::supportedLocales() const
{
    return m_supportedLocales;
}

QLocale TranslationManager::currentLocale() const
{
    return m_currentLocale;
}

bool TranslationManager::installTranslation(const QLocale &locale)
{
    // Remove previous translator if any
    if (m_translator) {
        qApp->removeTranslator(m_translator);
        delete m_translator;
        m_translator = nullptr;
    }

    // If locale is not in supported list (or is default English), keep English
    bool isSupported = std::ranges::any_of(m_supportedLocales,
                                           [&locale](const QLocale &l) { return l.language() == locale.language(); });

    if (!isSupported) {
        m_currentLocale = QLocale(QLocale::English);
        return true;
    }

    // Load translation from embedded resources at :/i18n
    m_translator = new QTranslator(qApp);
    if (m_translator->load(locale, u"KiraWavTar"_s, u"_"_s, u":/i18n"_s)) {
        qApp->installTranslator(m_translator);
        m_currentLocale = locale;
        return true;
    } else {
        qDebug() << "Failed to load translation for locale:" << locale.bcp47Name();
        delete m_translator;
        m_translator = nullptr;
        m_currentLocale = QLocale(QLocale::English);
        return false;
    }
}

void TranslationManager::setLangActionChecked(QMenu *i18nMenu, const QLocale &locale) const
{
    auto actions = i18nMenu->actions();
    for (auto action : std::as_const(actions)) {
        auto actionLocale = action->data().value<QLocale>();
        action->setChecked(actionLocale.language() == locale.language());
    }
}

void TranslationManager::saveUserLocaleSetting(const QLocale &locale) const
{
    QSettings settings;
    settings.setValue("locale", locale);
}

QLocale TranslationManager::getLocaleUserSetting() const
{
    QSettings settings;
    return settings.value("locale", QLocale::system()).value<QLocale>();
}

QMenu *TranslationManager::getI18nMenu()
{
    if (m_i18nMenu)
        return m_i18nMenu;

    m_i18nMenu = new QMenu("Language");

    // Add default English option
    auto defaultLang = new QAction("English (built-in)", m_i18nMenu);
    defaultLang->setData(QVariant::fromValue(QLocale(QLocale::English)));
    defaultLang->setCheckable(true);
    m_i18nMenu->addAction(defaultLang);

    // Add supported translations
    for (const auto &locale : std::as_const(m_supportedLocales)) {
        auto langAction = new QAction(
            QLatin1String("%1 (%2)").arg(QLocale::languageToString(locale.language()), locale.bcp47Name()), m_i18nMenu);
        langAction->setData(QVariant::fromValue(locale));
        langAction->setCheckable(true);
        m_i18nMenu->addAction(langAction);
    }

    QObject::connect(m_i18nMenu, &QMenu::triggered, m_i18nMenu, [this](QAction *action) {
        auto locale = action->data().value<QLocale>();
        installTranslation(locale);
        setLangActionChecked(m_i18nMenu, locale);
        saveUserLocaleSetting(locale);
    });

    setLangActionChecked(m_i18nMenu, m_currentLocale);
    return m_i18nMenu;
}
