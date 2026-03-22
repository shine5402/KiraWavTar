#ifndef CLI_CLIUTILS_H
#define CLI_CLIUTILS_H

#include <QRegularExpression>
#include <QString>

namespace cli {

// Strip HTML tags from preCheck report strings for plain-text CLI output
inline QString stripHtml(const QString &html)
{
    static const QRegularExpression reStyle("<style[^>]*>.*?</style>", QRegularExpression::DotMatchesEverythingOption);
    static const QRegularExpression reBr("<br\\s*/?>");
    static const QRegularExpression reP("<p[^>]*>");
    static const QRegularExpression reTag("<[^>]+>");

    QString text = html;
    text.remove(reStyle);
    text.replace(reBr, "\n");
    text.replace(reP, "");
    text.replace("</p>", "\n");
    text.remove(reTag);
    text = text.trimmed();
    return text;
}

} // namespace cli

#endif // CLI_CLIUTILS_H
