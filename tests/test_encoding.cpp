#include "Encoding.h"

#include <QTest>

using namespace PadForge;

class TestEncoding : public QObject {
    Q_OBJECT

private slots:
    void detectsUtf8Bom();
    void detectsUtf16LeBom();
    void detectsUtf16BeBom();
    void detectsUtf8Multibyte();
    void detectsAscii();
    void detectsLatin1Fallback();
    void detectsUtf16WithoutBom();
    void detectsBinary();

    void eolDetection();
    void lineCounting();

    void utf16RoundTrip();
    void utf8BomRoundTrip();
    void latin1RoundTrip();
    void latin1RejectsUnrepresentableText();
};

void TestEncoding::detectsUtf8Bom()
{
    const QByteArray data = QByteArrayLiteral("\xEF\xBB\xBF") + "héllo";
    const DetectionResult r = EncodingDetector::detect(data);
    QCOMPARE(r.encoding.charset, Charset::Utf8Bom);
    QVERIFY(r.encoding.bom);
    QCOMPARE(EncodingDetector::decode(data, r.encoding), QString::fromUtf8("héllo"));
}

void TestEncoding::detectsUtf16LeBom()
{
    const QString text = QStringLiteral("Hello");
    const QByteArray data = EncodingDetector::encode(text, {Charset::Utf16LE, true});
    QCOMPARE(data.left(2), QByteArrayLiteral("\xFF\xFE"));
    const DetectionResult r = EncodingDetector::detect(data);
    QCOMPARE(r.encoding.charset, Charset::Utf16LE);
    QCOMPARE(EncodingDetector::decode(data, r.encoding), text);
}

void TestEncoding::detectsUtf16BeBom()
{
    const QString text = QStringLiteral("Grüße");
    const QByteArray data = EncodingDetector::encode(text, {Charset::Utf16BE, true});
    QCOMPARE(data.left(2), QByteArrayLiteral("\xFE\xFF"));
    const DetectionResult r = EncodingDetector::detect(data);
    QCOMPARE(r.encoding.charset, Charset::Utf16BE);
    QCOMPARE(EncodingDetector::decode(data, r.encoding), text);
}

void TestEncoding::detectsUtf8Multibyte()
{
    const QByteArray data = QStringLiteral("naïve — café 日本語").toUtf8();
    const DetectionResult r = EncodingDetector::detect(data);
    QCOMPARE(r.encoding.charset, Charset::Utf8);
    QVERIFY(!r.encoding.bom);
}

void TestEncoding::detectsAscii()
{
    const QByteArray data = "plain ascii text 12345\nsecond line";
    const DetectionResult r = EncodingDetector::detect(data);
    QCOMPARE(r.encoding.charset, Charset::Ascii);
}

void TestEncoding::detectsLatin1Fallback()
{
    // 0xE9 is a lone high byte: valid Latin-1, invalid UTF-8.
    const QByteArray data = QByteArray("caf") + char(0xE9);
    const DetectionResult r = EncodingDetector::detect(data);
    QCOMPARE(r.encoding.charset, Charset::Latin1);
    QCOMPARE(EncodingDetector::decode(data, r.encoding), QString::fromUtf8("café"));
}

void TestEncoding::detectsUtf16WithoutBom()
{
    // ASCII "HELLO" encoded as UTF-16LE without a BOM: alternating NULs.
    const QByteArray data = QByteArrayLiteral("H\0E\0L\0L\0O\0");
    const DetectionResult r = EncodingDetector::detect(data);
    QCOMPARE(r.encoding.charset, Charset::Utf16LE);
    QCOMPARE(EncodingDetector::decode(data, r.encoding), QStringLiteral("HELLO"));
}

void TestEncoding::detectsBinary()
{
    QByteArray data;
    data.append(char(0x7F));
    data.append('\0');
    data.append(char(0x01));
    data.append(char(0x02));
    QVERIFY(EncodingDetector::looksBinary(data));
}

void TestEncoding::eolDetection()
{
    QCOMPARE(EncodingDetector::detectEol("a\nb\nc"), Eol::Lf);
    QCOMPARE(EncodingDetector::detectEol("a\r\nb\r\n"), Eol::CrLf);
    QCOMPARE(EncodingDetector::detectEol("a\rb\rc"), Eol::Cr);
    QCOMPARE(EncodingDetector::detectEol("a\r\nb\nc"), Eol::Mixed);
    QCOMPARE(EncodingDetector::detectEol("no newline"), Eol::None);
}

void TestEncoding::lineCounting()
{
    QCOMPARE(EncodingDetector::countLines(QByteArray()), 0LL);
    QCOMPARE(EncodingDetector::countLines("one"), 1LL);
    QCOMPARE(EncodingDetector::countLines("one\ntwo\nthree"), 3LL);
    QCOMPARE(EncodingDetector::countLines("one\ntwo\n"), 3LL);
}

void TestEncoding::utf16RoundTrip()
{
    const QString original = QStringLiteral("Line 1\r\nLine 2 — ünïcode ✓");
    const QByteArray le = EncodingDetector::encode(original, {Charset::Utf16LE, true});
    QCOMPARE(EncodingDetector::decode(le, {Charset::Utf16LE, true}), original);
    const QByteArray be = EncodingDetector::encode(original, {Charset::Utf16BE, true});
    QCOMPARE(EncodingDetector::decode(be, {Charset::Utf16BE, true}), original);
}

void TestEncoding::utf8BomRoundTrip()
{
    const QString original = QStringLiteral("tést");
    const QByteArray encoded = EncodingDetector::encode(original, {Charset::Utf8Bom, true});
    QVERIFY(encoded.startsWith(QByteArrayLiteral("\xEF\xBB\xBF")));
    QCOMPARE(EncodingDetector::decode(encoded, {Charset::Utf8Bom, true}), original);
}

void TestEncoding::latin1RoundTrip()
{
    const QString original = QStringLiteral("café résumé");
    const QByteArray encoded = EncodingDetector::encode(original, {Charset::Latin1, false});
    const QByteArray expected("caf\xE9 r\xE9sum\xE9"); // raw Latin-1 bytes
    QCOMPARE(encoded, expected);
    QCOMPARE(EncodingDetector::decode(encoded, {Charset::Latin1, false}), original);
}

void TestEncoding::latin1RejectsUnrepresentableText()
{
    bool representable = true;
    const QByteArray encoded = EncodingDetector::encode(
        QStringLiteral("snowman \u2603"), {Charset::Latin1, false}, &representable);
    QVERIFY(!representable);
    QVERIFY(encoded.isEmpty());
}

QTEST_MAIN(TestEncoding)
#include "test_encoding.moc"
