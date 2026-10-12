#include "Outline.h"

#include <QRegularExpression>
#include <QSet>
#include <QStringList>

namespace PadForge {

namespace {

const QSet<QString>& cLikeLanguages() {
    static const QSet<QString> langs = {
        QStringLiteral("c"),          QStringLiteral("cpp"),        QStringLiteral("c++"),
        QStringLiteral("java"),       QStringLiteral("javascript"), QStringLiteral("js"),
        QStringLiteral("typescript"), QStringLiteral("ts"),         QStringLiteral("csharp"),
        QStringLiteral("cs"),         QStringLiteral("go"),         QStringLiteral("rust"),
        QStringLiteral("php"),        QStringLiteral("kotlin"),     QStringLiteral("swift"),
        QStringLiteral("d"),          QStringLiteral("objc"),       QStringLiteral("objective-c"),
        QStringLiteral("perl"),       QStringLiteral("r"),          QStringLiteral("scala"),
        QStringLiteral("groovy"),     QStringLiteral("dart"),
    };
    return langs;
}

const QSet<QString>& controlKeywords() {
    static const QSet<QString> keywords = {
        QStringLiteral("if"),           QStringLiteral("for"),     QStringLiteral("while"),
        QStringLiteral("switch"),       QStringLiteral("return"),  QStringLiteral("catch"),
        QStringLiteral("do"),           QStringLiteral("else"),    QStringLiteral("sizeof"),
        QStringLiteral("delete"),       QStringLiteral("new"),     QStringLiteral("throw"),
        QStringLiteral("case"),         QStringLiteral("typedef"), QStringLiteral("using"),
        QStringLiteral("define"),       QStringLiteral("assert"),  QStringLiteral("expect"),
        QStringLiteral("synchronized"), QStringLiteral("foreach"), QStringLiteral("when"),
    };
    return keywords;
}

QString firstToken(const QString& trimmed) {
    int end = 0;
    while (end < trimmed.size() &&
           (trimmed.at(end).isLetterOrNumber() || trimmed.at(end) == QLatin1Char('_')))
        ++end;
    return trimmed.left(end);
}

int indentLevel(const QString& line) {
    int indent = 0;
    for (const QChar ch : line) {
        if (ch == QLatin1Char(' '))
            ++indent;
        else if (ch == QLatin1Char('\t'))
            indent += 4;
        else
            break;
    }
    return indent / 4;
}

QList<OutlineItem> parseCLike(const QStringList& lines) {
    static const QRegularExpression containerRe(
        QStringLiteral("^\\s*(class|struct|enum|namespace|interface|union|trait|impl|record|"
                       "object|module|protocol|extension)\\s+([A-Za-z_]\\w*)"));
    static const QRegularExpression functionRe(
        QStringLiteral("^\\s*(?:(?:public|private|protected|internal|static|virtual|inline|"
                       "explicit|constexpr|consteval|constinit|friend|extern|abstract|final|"
                       "async|export|default|sealed|partial|override|synchronized|native|"
                       "constructor|function)\\s+)*(?:[A-Za-z_][\\w:<>,.*&\\s\\[\\]]*\\s+)?"
                       "([A-Za-z_]\\w*)\\s*\\(([^;{}()]*)\\)\\s*(?:const\\s*)?"
                       "(?:noexcept(?:\\s*\\([^)]*\\))?\\s*)?(?:override\\s*)?(?:final\\s*)?"
                       "(?:->\\s*[^;{]+?)?\\s*(?:\\{|;|$)"));

    QList<OutlineItem> items;
    for (int i = 0; i < lines.size(); ++i) {
        const QString& line = lines.at(i);
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith(QStringLiteral("//")) ||
            trimmed.startsWith(QStringLiteral("#")) || trimmed.startsWith(QStringLiteral("*")))
            continue;

        QRegularExpressionMatch match = containerRe.match(line);
        if (match.hasMatch()) {
            OutlineItem item;
            item.name = match.captured(2);
            item.kind = match.captured(1);
            item.signature = item.name;
            item.line = i + 1;
            item.level = indentLevel(line);
            items.append(item);
            continue;
        }

        if (firstToken(trimmed) == QStringLiteral("return") ||
            firstToken(trimmed) == QStringLiteral("throw") ||
            firstToken(trimmed) == QStringLiteral("new") ||
            firstToken(trimmed) == QStringLiteral("delete"))
            continue;

        match = functionRe.match(line);
        if (match.hasMatch()) {
            const QString name = match.captured(1);
            if (controlKeywords().contains(name))
                continue;
            OutlineItem item;
            item.name = name;
            item.kind = QStringLiteral("function");
            item.signature =
                name + QLatin1Char('(') + match.captured(2).trimmed() + QLatin1Char(')');
            item.line = i + 1;
            item.level = indentLevel(line);
            items.append(item);
        }
    }
    return items;
}

QList<OutlineItem> parsePython(const QStringList& lines) {
    static const QRegularExpression defRe(
        QStringLiteral("^(\\s*)(?:async\\s+)?def\\s+([A-Za-z_]\\w*)\\s*\\(([^)]*)\\)"));
    static const QRegularExpression classRe(QStringLiteral("^(\\s*)class\\s+([A-Za-z_]\\w*)"));

    QList<OutlineItem> items;
    for (int i = 0; i < lines.size(); ++i) {
        const QString& line = lines.at(i);
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith(QLatin1Char('#')))
            continue;

        QRegularExpressionMatch match = classRe.match(line);
        if (match.hasMatch()) {
            OutlineItem item;
            item.name = match.captured(2);
            item.kind = QStringLiteral("class");
            item.signature = item.name;
            item.line = i + 1;
            item.level = indentLevel(line);
            items.append(item);
            continue;
        }

        match = defRe.match(line);
        if (match.hasMatch()) {
            OutlineItem item;
            item.name = match.captured(2);
            item.kind = QStringLiteral("function");
            item.signature =
                item.name + QLatin1Char('(') + match.captured(3).trimmed() + QLatin1Char(')');
            item.line = i + 1;
            item.level = indentLevel(line);
            items.append(item);
        }
    }
    return items;
}

QList<OutlineItem> parseShell(const QStringList& lines) {
    static const QRegularExpression functionRe(
        QStringLiteral("^\\s*(?:function\\s+)?([A-Za-z_]\\w*)\\s*\\(\\s*\\)"));

    QList<OutlineItem> items;
    for (int i = 0; i < lines.size(); ++i) {
        QRegularExpressionMatch match = functionRe.match(lines.at(i));
        if (match.hasMatch()) {
            OutlineItem item;
            item.name = match.captured(1);
            item.kind = QStringLiteral("function");
            item.signature = item.name + QStringLiteral("()");
            item.line = i + 1;
            item.level = indentLevel(lines.at(i));
            items.append(item);
        }
    }
    return items;
}

QList<OutlineItem> parseMarkdown(const QStringList& lines) {
    static const QRegularExpression headingRe(QStringLiteral("^(#{1,6})\\s+(.+?)\\s*#*\\s*$"));

    QList<OutlineItem> items;
    bool inFence = false;
    for (int i = 0; i < lines.size(); ++i) {
        const QString& line = lines.at(i);
        if (line.trimmed().startsWith(QStringLiteral("```"))) {
            inFence = !inFence;
            continue;
        }
        if (inFence)
            continue;
        QRegularExpressionMatch match = headingRe.match(line);
        if (match.hasMatch()) {
            OutlineItem item;
            item.name = match.captured(2).trimmed();
            item.kind = QStringLiteral("section");
            item.signature = item.name;
            item.line = i + 1;
            item.level = match.captured(1).size() - 1;
            items.append(item);
        }
    }
    return items;
}

} // namespace

bool OutlineParser::supports(const QString& languageId) {
    const QString id = languageId.toLower();
    return cLikeLanguages().contains(id) || id == QStringLiteral("python") ||
           id == QStringLiteral("bash") || id == QStringLiteral("shell") ||
           id == QStringLiteral("sh") || id == QStringLiteral("markdown");
}

QList<OutlineItem> OutlineParser::parse(const QString& text, const QString& languageId) {
    const QString id = languageId.toLower();
    if (id == QStringLiteral("python"))
        return parsePython(text.split(QLatin1Char('\n')));
    if (id == QStringLiteral("bash") || id == QStringLiteral("shell") || id == QStringLiteral("sh"))
        return parseShell(text.split(QLatin1Char('\n')));
    if (id == QStringLiteral("markdown"))
        return parseMarkdown(text.split(QLatin1Char('\n')));
    if (cLikeLanguages().contains(id))
        return parseCLike(text.split(QLatin1Char('\n')));
    return {};
}

} // namespace PadForge
