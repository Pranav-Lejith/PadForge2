#include "Outline.h"

#include <QTest>

using namespace PadForge;

class OutlineTest : public QObject {
    Q_OBJECT

private slots:
    void cppOutline();
    void pythonOutline();
    void markdownOutline();
    void ignoresControlKeywords();
    void unsupportedLanguage();
};

void OutlineTest::cppOutline() {
    const QString source = QStringLiteral("#include <vector>\n"
                                          "\n"
                                          "namespace demo {\n"
                                          "\n"
                                          "class Widget {\n"
                                          "public:\n"
                                          "    Widget();\n"
                                          "    int width() const;\n"
                                          "    virtual void resize(int w, int h) override;\n"
                                          "};\n"
                                          "\n"
                                          "int add(int a, int b) {\n"
                                          "    return a + b;\n"
                                          "}\n"
                                          "\n"
                                          "} // namespace demo\n");

    const QList<OutlineItem> items = OutlineParser::parse(source, QStringLiteral("cpp"));

    QStringList names;
    for (const OutlineItem& item : items)
        names << item.name;
    QCOMPARE(names, QStringList({QStringLiteral("demo"), QStringLiteral("Widget"),
                                 QStringLiteral("Widget"), QStringLiteral("width"),
                                 QStringLiteral("resize"), QStringLiteral("add")}));

    QCOMPARE(items.at(0).kind, QStringLiteral("namespace"));
    QCOMPARE(items.at(1).kind, QStringLiteral("class"));
    QCOMPARE(items.at(1).level, 0);
    QCOMPARE(items.at(3).name, QStringLiteral("width"));
    QCOMPARE(items.at(5).line, 12);
    QCOMPARE(items.at(5).signature, QStringLiteral("add(int a, int b)"));
}

void OutlineTest::pythonOutline() {
    const QString source = QStringLiteral("class Greeter:\n"
                                          "    def hello(self, name):\n"
                                          "        return name\n"
                                          "\n"
                                          "    async def goodbye(self):\n"
                                          "        pass\n"
                                          "\n"
                                          "def main():\n"
                                          "    pass\n");

    const QList<OutlineItem> items = OutlineParser::parse(source, QStringLiteral("python"));
    QCOMPARE(items.size(), 4);
    QCOMPARE(items.at(0).name, QStringLiteral("Greeter"));
    QCOMPARE(items.at(0).kind, QStringLiteral("class"));
    QCOMPARE(items.at(1).name, QStringLiteral("hello"));
    QCOMPARE(items.at(1).level, 1);
    QCOMPARE(items.at(1).signature, QStringLiteral("hello(self, name)"));
    QCOMPARE(items.at(2).name, QStringLiteral("goodbye"));
    QCOMPARE(items.at(3).name, QStringLiteral("main"));
    QCOMPARE(items.at(3).line, 8);
}

void OutlineTest::markdownOutline() {
    const QString source = QStringLiteral("# Title\n"
                                          "text\n"
                                          "## Section\n"
                                          "```\n"
                                          "# not a heading\n"
                                          "```\n"
                                          "### Deep\n");

    const QList<OutlineItem> items = OutlineParser::parse(source, QStringLiteral("markdown"));
    QCOMPARE(items.size(), 3);
    QCOMPARE(items.at(0).name, QStringLiteral("Title"));
    QCOMPARE(items.at(0).level, 0);
    QCOMPARE(items.at(1).name, QStringLiteral("Section"));
    QCOMPARE(items.at(1).level, 1);
    QCOMPARE(items.at(2).name, QStringLiteral("Deep"));
}

void OutlineTest::ignoresControlKeywords() {
    const QString source = QStringLiteral("int compute(int x) {\n"
                                          "    if (x > 0) {\n"
                                          "        return x;\n"
                                          "    }\n"
                                          "    for (int i = 0; i < 3; ++i) {\n"
                                          "    }\n"
                                          "    return 0;\n"
                                          "}\n");

    const QList<OutlineItem> items = OutlineParser::parse(source, QStringLiteral("cpp"));
    QStringList names;
    for (const OutlineItem& item : items)
        names << item.name;
    QCOMPARE(names, QStringList({QStringLiteral("compute")}));
}

void OutlineTest::unsupportedLanguage() {
    QVERIFY(!OutlineParser::supports(QStringLiteral("text")));
    QVERIFY(OutlineParser::supports(QStringLiteral("cpp")));
    QVERIFY(OutlineParser::parse(QStringLiteral("anything"), QStringLiteral("text")).isEmpty());
}

QTEST_MAIN(OutlineTest)
#include "test_outline.moc"
