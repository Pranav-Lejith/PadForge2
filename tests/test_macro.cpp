#include "Macro.h"

#include <QTemporaryDir>
#include <QTest>

using namespace PadForge;

class TestMacro : public QObject {
    Q_OBJECT

private slots:
    void recordAndStop();
    void recordIgnoredWhenNotRecording();
    void saveAndLoad();
};

void TestMacro::recordAndStop()
{
    MacroRecorder recorder;
    QVERIFY(!recorder.isRecording());
    recorder.start("My Macro");
    QVERIFY(recorder.isRecording());

    MacroStep insert;
    insert.kind = MacroStep::Kind::InsertText;
    insert.text = "hello";
    recorder.record(insert);

    MacroStep command;
    command.kind = MacroStep::Kind::Command;
    command.command = "edit.copy";
    recorder.record(command);

    const Macro macro = recorder.stop();
    QVERIFY(!recorder.isRecording());
    QCOMPARE(macro.name, QString("My Macro"));
    QCOMPARE(macro.steps.size(), 2);
    QCOMPARE(macro.steps[0].text, QString("hello"));
    QCOMPARE(macro.steps[1].command, QString("edit.copy"));
}

void TestMacro::recordIgnoredWhenNotRecording()
{
    MacroRecorder recorder;
    MacroStep step;
    step.text = "ignored";
    recorder.record(step);
    QCOMPARE(recorder.current().steps.size(), 0);
}

void TestMacro::saveAndLoad()
{
    Macro macro;
    macro.name = "Demo";
    macro.shortcut = "Ctrl+Shift+D";
    MacroStep step;
    step.kind = MacroStep::Kind::ReplaceText;
    step.text = "world";
    step.position = 5;
    step.length = 5;
    macro.steps.append(step);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + "/macros.json";
    QVERIFY(MacroLibrary::save({macro}, path));

    bool ok = false;
    const QList<Macro> loaded = MacroLibrary::load(path, &ok);
    QVERIFY(ok);
    QCOMPARE(loaded.size(), 1);
    QCOMPARE(loaded[0].name, QString("Demo"));
    QCOMPARE(loaded[0].shortcut, QString("Ctrl+Shift+D"));
    QCOMPARE(loaded[0].steps.size(), 1);
    QCOMPARE(loaded[0].steps[0].kind, MacroStep::Kind::ReplaceText);
    QCOMPARE(loaded[0].steps[0].text, QString("world"));
    QCOMPARE(loaded[0].steps[0].position, 5);
}

QTEST_MAIN(TestMacro)
#include "test_macro.moc"
