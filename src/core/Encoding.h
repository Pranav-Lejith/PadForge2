#pragma once

#include <QByteArray>
#include <QString>

namespace PadForge {

enum class Charset {
    Ascii,
    Utf8,
    Utf8Bom,
    Utf16LE,
    Utf16BE,
    Utf32LE,
    Utf32BE,
    Latin1,
};

enum class Eol {
    None,
    Lf,
    CrLf,
    Cr,
    Mixed,
};

struct EncodingInfo {
    Charset charset = Charset::Utf8;
    bool bom = false;

    QString displayName() const;
    QString canonicalName() const;
    bool isUnicode() const;
};

struct DetectionResult {
    EncodingInfo encoding;
    Eol eol = Eol::None;
    bool binary = false;
};

// Stateless helpers used by both the GUI and the test suite.
class EncodingDetector {
public:
    // Detect charset (via BOM, UTF-8 validation, UTF-16 heuristics, Latin-1
    // fallback), line-ending style and a rough binary flag for `sample`.
    static DetectionResult detect(const QByteArray& sample);

    static Eol detectEol(const QByteArray& data);
    static qint64 countLines(const QByteArray& data);
    static bool looksBinary(const QByteArray& sample);
    static bool isValidUtf8(const QByteArray& data);

    // BOM aware decoding to a UTF-8 byte array and to QString.
    static QString decode(const QByteArray& raw, const EncodingInfo& enc);
    static QByteArray decodeToUtf8(const QByteArray& raw, const EncodingInfo& enc);

    // Encode UTF-8 (or a QString) back to raw bytes honouring charset + BOM.
    static QByteArray encode(const QString& text, const EncodingInfo& enc);
    static QByteArray encodeFromUtf8(const QByteArray& utf8, const EncodingInfo& enc);

    static Charset charsetFromName(const QString& name);
    static QString charsetName(Charset c);
    static QString eolName(Eol eol);
    static QByteArray eolBytes(Eol eol);

    static const int kBomLength[8];
};

} // namespace PadForge
