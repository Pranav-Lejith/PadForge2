#include <QtTest>

#include "PluginHost.h"

#include <QMap>
#include <functional>

using namespace PadForge;

namespace {

class FakeContext : public PluginContext {
public:
    QString text;
    QString language;
    QString lastMessage;
    QMap<QString, std::function<void()>> commands;

    void insertText(const QString& t) override { text += t; }
    void setDocumentText(const QString& t) override { text = t; }
    QString documentText() const override { return text; }
    int lineCount() const override { return text.isEmpty() ? 0 : text.count(QLatin1Char('\n')) + 1; }
    int currentLine() const override { return 1; }
    void gotoLine(int) override {}
    void setLanguage(const QString& id) override { language = id; }
    void showMessage(const QString& message) override { lastMessage = message; }
    void addMenuItem(const QString& name, std::function<void()> callback) override
    {
        commands.insert(name, std::move(callback));
    }
};

} // namespace

class PluginTest : public QObject {
    Q_OBJECT

private slots:
    void loadsSamplePlugins();
    void wordCountCommand();
    void trimTrailingWhitespaceCommand();
    void sortLinesCommand();
    void replEvaluates();
    void replExpressionRetry();
    void replRoutesPrintThroughOutputSink();
    void replSurfacesErrors();
};

void PluginTest::loadsSamplePlugins()
{
    FakeContext context;
    PluginHost host(&context);
    QVERIFY(host.hasLua());

    const int loaded = host.loadLuaPluginsIn(QStringLiteral(PADFORGE_TEST_PLUGIN_DIR));
    QCOMPARE(loaded, 6);
    QCOMPARE(context.commands.size(), 6);
    QVERIFY(context.commands.contains(QStringLiteral("Word Count")));
    QVERIFY(context.commands.contains(QStringLiteral("Sort Lines")));
}

void PluginTest::wordCountCommand()
{
    FakeContext context;
    context.text = QStringLiteral("alpha beta\ngamma\n");
    PluginHost host(&context);
    host.loadLuaPluginsIn(QStringLiteral(PADFORGE_TEST_PLUGIN_DIR));

    QVERIFY(context.commands.contains(QStringLiteral("Word Count")));
    context.commands.value(QStringLiteral("Word Count"))();
    QVERIFY(context.lastMessage.contains(QStringLiteral("3 words")));
}

void PluginTest::trimTrailingWhitespaceCommand()
{
    FakeContext context;
    context.text = QStringLiteral("a  \nb\t\n");
    PluginHost host(&context);
    host.loadLuaPluginsIn(QStringLiteral(PADFORGE_TEST_PLUGIN_DIR));

    context.commands.value(QStringLiteral("Trim Trailing Whitespace"))();
    QCOMPARE(context.text, QStringLiteral("a\nb\n"));
}

void PluginTest::sortLinesCommand()
{
    FakeContext context;
    context.text = QStringLiteral("banana\napple\ncherry\n");
    PluginHost host(&context);
    host.loadLuaPluginsIn(QStringLiteral(PADFORGE_TEST_PLUGIN_DIR));

    context.commands.value(QStringLiteral("Sort Lines"))();
    QCOMPARE(context.text, QStringLiteral("apple\nbanana\ncherry\n"));
}

void PluginTest::replEvaluates()
{
    FakeContext context;
    PluginHost host(&context);
    QVERIFY(host.hasLua());

    QString result;
    QString error;
    QVERIFY(host.evaluateLua(QStringLiteral("g = 40"), nullptr, &error));
    QVERIFY(error.isEmpty());
    QVERIFY(host.evaluateLua(QStringLiteral("g + 2"), &result, &error));
    QCOMPARE(result, QStringLiteral("42"));

    context.text = QStringLiteral("alpha\nbeta");
    QVERIFY(host.evaluateLua(QStringLiteral("padforge.line_count()"), &result));
    QCOMPARE(result, QStringLiteral("2"));
}

void PluginTest::replExpressionRetry()
{
    FakeContext context;
    PluginHost host(&context);

    QString result;
    QVERIFY(host.evaluateLua(QStringLiteral("1+1"), &result));
    QCOMPARE(result, QStringLiteral("2"));

    QVERIFY(host.evaluateLua(QStringLiteral("(3 * 4) + 1"), &result));
    QCOMPARE(result, QStringLiteral("13"));

    QVERIFY(host.evaluateLua(QStringLiteral("'a' .. 'b'"), &result));
    QCOMPARE(result, QStringLiteral("ab"));
}

void PluginTest::replRoutesPrintThroughOutputSink()
{
    FakeContext context;
    PluginHost host(&context);

    QStringList printed;
    host.setOutputSink([&printed](const QString& line) { printed.append(line); });

    QVERIFY(host.evaluateLua(QStringLiteral("print('hello', 42)")));
    QCOMPARE(printed, QStringList({QStringLiteral("hello\t42")}));
}

void PluginTest::replSurfacesErrors()
{
    FakeContext context;
    PluginHost host(&context);

    QString error;
    QVERIFY(!host.evaluateLua(QStringLiteral("this is not lua"), nullptr, &error));
    QVERIFY(!error.isEmpty());
}

QTEST_MAIN(PluginTest)
#include "test_plugin.moc"
