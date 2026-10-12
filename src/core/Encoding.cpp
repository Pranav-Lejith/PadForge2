#include "Encoding.h"

#include <QVector>

#include <algorithm>
#include <cstring>

namespace PadForge {

namespace {

constexpr int kSampleLimit = 64 * 1024;

bool hasBom(const QByteArray& d, const char* bom, int len)
{
    return d.size() >= len && std::memcmp(d.constData(), bom, size_t(len)) == 0;
}

Eol eolFromText(const QString& text)
{
    bool crlf = false;
    bool lf = false;
    bool cr = false;
    const int n = int(text.size());
    for (int i = 0; i < n; ++i) {
        const QChar c = text.at(i);
        if (c == u'\r') {
            if (i + 1 < n && text.at(i + 1) == u'\n') {
                crlf = true;
                ++i;
            } else {
                cr = true;
            }
        } else if (c == u'\n') {
            lf = true;
        }
    }
    const int kinds = int(crlf) + int(lf) + int(cr);
    if (kinds == 0)
        return Eol::None;
    if (kinds > 1)
        return Eol::Mixed;
    if (crlf)
        return Eol::CrLf;
    if (cr)
        return Eol::Cr;
    return Eol::Lf;
}

QString decodeUtf16(const QByteArray& raw, bool bigEndian)
{
    const auto* p = reinterpret_cast<const unsigned char*>(raw.constData());
    const int bytes = int(raw.size());
    QVector<char16_t> units;
    units.reserve(bytes / 2);
    for (int i = 0; i + 1 < bytes; i += 2) {
        char16_t u = bigEndian ? char16_t((p[i] << 8) | p[i + 1])
                               : char16_t((p[i + 1] << 8) | p[i]);
        units.append(u);
    }
    int start = 0;
    if (!units.isEmpty() && units.first() == 0xFEFF)
        start = 1;
    return QString::fromUtf16(units.constData() + start, units.size() - start);
}

QString decodeUtf32(const QByteArray& raw, bool bigEndian)
{
    const auto* p = reinterpret_cast<const unsigned char*>(raw.constData());
    const int bytes = int(raw.size());
    QString out;
    out.reserve(bytes / 4);
    for (int i = 0; i + 3 < bytes; i += 4) {
        quint32 cp = bigEndian ? (quint32(p[i]) << 24) | (quint32(p[i + 1]) << 16)
                                    | (quint32(p[i + 2]) << 8) | quint32(p[i + 3])
                              : (quint32(p[i + 3]) << 24) | (quint32(p[i + 2]) << 16)
                                    | (quint32(p[i + 1]) << 8) | quint32(p[i]);
        if (cp == 0xFEFF)
            continue;
        if (cp <= 0xFFFF) {
            out.append(QChar(char16_t(cp)));
        } else {
            cp -= 0x10000;
            out.append(QChar(char16_t(0xD800 + (cp >> 10))));
            out.append(QChar(char16_t(0xDC00 + (cp & 0x3FF))));
        }
    }
    return out;
}

QByteArray encodeUtf16(const QString& text, bool bigEndian, bool bom)
{
    QByteArray out;
    out.reserve(text.size() * 2 + 2);
    auto put = [&](char16_t u) {
        if (bigEndian) {
            out.append(char((u >> 8) & 0xFF));
            out.append(char(u & 0xFF));
        } else {
            out.append(char(u & 0xFF));
            out.append(char((u >> 8) & 0xFF));
        }
    };
    if (bom)
        put(0xFEFF);
    for (QChar c : text)
        put(c.unicode());
    return out;
}

QByteArray encodeUtf32(const QString& text, bool bigEndian, bool bom)
{
    QByteArray out;
    out.reserve(text.size() * 4 + 4);
    auto put = [&](quint32 cp) {
        if (bigEndian) {
            out.append(char((cp >> 24) & 0xFF));
            out.append(char((cp >> 16) & 0xFF));
            out.append(char((cp >> 8) & 0xFF));
            out.append(char(cp & 0xFF));
        } else {
            out.append(char(cp & 0xFF));
            out.append(char((cp >> 8) & 0xFF));
            out.append(char((cp >> 16) & 0xFF));
            out.append(char((cp >> 24) & 0xFF));
        }
    };
    if (bom)
        put(0xFEFF);
    const int n = int(text.size());
    for (int i = 0; i < n; ++i) {
        const char16_t u = text.at(i).unicode();
        if (u >= 0xD800 && u <= 0xDBFF && i + 1 < n) {
            const char16_t lo = text.at(i + 1).unicode();
            if (lo >= 0xDC00 && lo <= 0xDFFF) {
                const quint32 cp = 0x10000 + ((quint32(u - 0xD800) << 10) | (lo - 0xDC00));
                put(cp);
                ++i;
                continue;
            }
        }
        put(u);
    }
    return out;
}

} // namespace

const int EncodingDetector::kBomLength[8] = {0, 0, 3, 2, 2, 4, 4, 0};

QString EncodingInfo::displayName() const
{
    switch (charset) {
    case Charset::Ascii:
        return QStringLiteral("ANSI/ASCII");
    case Charset::Utf8:
        return QStringLiteral("UTF-8");
    case Charset::Utf8Bom:
        return QStringLiteral("UTF-8 (BOM)");
    case Charset::Utf16LE:
        return QStringLiteral("UTF-16 LE");
    case Charset::Utf16BE:
        return QStringLiteral("UTF-16 BE");
    case Charset::Utf32LE:
        return QStringLiteral("UTF-32 LE");
    case Charset::Utf32BE:
        return QStringLiteral("UTF-32 BE");
    case Charset::Latin1:
        return QStringLiteral("Latin-1 (ISO-8859-1)");
    }
    return QStringLiteral("Unknown");
}

QString EncodingInfo::canonicalName() const
{
    return EncodingDetector::charsetName(charset);
}

bool EncodingInfo::isUnicode() const
{
    return charset == Charset::Utf8 || charset == Charset::Utf8Bom
        || charset == Charset::Utf16LE || charset == Charset::Utf16BE
        || charset == Charset::Utf32LE || charset == Charset::Utf32BE;
}

bool EncodingDetector::isValidUtf8(const QByteArray& data)
{
    const auto* p = reinterpret_cast<const unsigned char*>(data.constData());
    const int n = int(data.size());
    int i = 0;
    while (i < n) {
        const unsigned char c = p[i];
        int extra = 0;
        unsigned int cp = 0;
        if (c < 0x80) {
            ++i;
            continue;
        } else if ((c & 0xE0) == 0xC0) {
            extra = 1;
            cp = c & 0x1F;
        } else if ((c & 0xF0) == 0xE0) {
            extra = 2;
            cp = c & 0x0F;
        } else if ((c & 0xF8) == 0xF0) {
            extra = 3;
            cp = c & 0x07;
        } else {
            return false;
        }
        if (i + extra >= n)
            return false;
        for (int k = 1; k <= extra; ++k) {
            const unsigned char cc = p[i + k];
            if ((cc & 0xC0) != 0x80)
                return false;
            cp = (cp << 6) | (cc & 0x3F);
        }
        if (extra == 1 && cp < 0x80)
            return false;
        if (extra == 2 && cp < 0x800)
            return false;
        if (extra == 3 && cp < 0x10000)
            return false;
        if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
            return false;
        i += extra + 1;
    }
    return true;
}

bool EncodingDetector::looksBinary(const QByteArray& sample)
{
    if (sample.isEmpty())
        return false;
    int control = 0;
    int nul = 0;
    const int n = std::min(int(sample.size()), kSampleLimit);
    for (int i = 0; i < n; ++i) {
        const unsigned char c = static_cast<unsigned char>(sample.at(i));
        if (c == 0)
            ++nul;
        else if (c < 0x09 || (c > 0x0D && c < 0x20))
            ++control;
    }
    if (nul > 0)
        return true;
    return control * 100 > n * 5;
}

Eol EncodingDetector::detectEol(const QByteArray& data)
{
    bool crlf = false;
    bool lf = false;
    bool cr = false;
    const int n = int(data.size());
    for (int i = 0; i < n; ++i) {
        const char c = data.at(i);
        if (c == '\r') {
            if (i + 1 < n && data.at(i + 1) == '\n') {
                crlf = true;
                ++i;
            } else {
                cr = true;
            }
        } else if (c == '\n') {
            lf = true;
        }
    }
    const int kinds = int(crlf) + int(lf) + int(cr);
    if (kinds == 0)
        return Eol::None;
    if (kinds > 1)
        return Eol::Mixed;
    if (crlf)
        return Eol::CrLf;
    if (cr)
        return Eol::Cr;
    return Eol::Lf;
}

qint64 EncodingDetector::countLines(const QByteArray& data)
{
    if (data.isEmpty())
        return 0;
    qint64 lines = 1;
    for (char c : data)
        if (c == '\n')
            ++lines;
    return lines;
}

DetectionResult EncodingDetector::detect(const QByteArray& sample)
{
    DetectionResult result;
    if (sample.isEmpty()) {
        result.encoding = {Charset::Utf8, false};
        return result;
    }

    // 1. Byte-order marks (longest first so UTF-32 wins over UTF-16).
    if (hasBom(sample, "\x00\x00\xFE\xFF", 4)) {
        result.encoding = {Charset::Utf32BE, true};
    } else if (hasBom(sample, "\xFF\xFE\x00\x00", 4)) {
        result.encoding = {Charset::Utf32LE, true};
    } else if (hasBom(sample, "\xEF\xBB\xBF", 3)) {
        result.encoding = {Charset::Utf8Bom, true};
    } else if (hasBom(sample, "\xFE\xFF", 2)) {
        result.encoding = {Charset::Utf16BE, true};
    } else if (hasBom(sample, "\xFF\xFE", 2)) {
        result.encoding = {Charset::Utf16LE, true};
    } else {
        // 2. Heuristic detection of unmarked UTF-16 via alternating NUL bytes.
        const int probe = std::min(int(sample.size()), 4096);
        int evenNul = 0;
        int oddNul = 0;
        int pairs = 0;
        for (int i = 0; i + 1 < probe; i += 2) {
            if (sample.at(i) == '\0')
                ++evenNul;
            if (sample.at(i + 1) == '\0')
                ++oddNul;
            ++pairs;
        }
        const bool le = pairs > 0 && oddNul > pairs / 2;
        const bool be = pairs > 0 && evenNul > pairs / 2;
        if (le && !be) {
            result.encoding = {Charset::Utf16LE, false};
        } else if (be && !le) {
            result.encoding = {Charset::Utf16BE, false};
        } else if (looksBinary(sample)) {
            result.binary = true;
            result.encoding = {Charset::Latin1, false};
        } else if (isValidUtf8(sample)) {
            bool hasMultibyte = false;
            for (unsigned char c : sample) {
                if (c & 0x80) {
                    hasMultibyte = true;
                    break;
                }
            }
            result.encoding = {hasMultibyte ? Charset::Utf8 : Charset::Ascii, false};
        } else {
            result.encoding = {Charset::Latin1, false};
        }
    }

    if (result.encoding.charset == Charset::Ascii)
        result.encoding.charset = Charset::Ascii;
    result.eol = eolFromText(decode(sample, result.encoding));
    return result;
}

QString EncodingDetector::decode(const QByteArray& raw, const EncodingInfo& enc)
{
    switch (enc.charset) {
    case Charset::Utf16LE:
        return decodeUtf16(raw, false);
    case Charset::Utf16BE:
        return decodeUtf16(raw, true);
    case Charset::Utf32LE:
        return decodeUtf32(raw, false);
    case Charset::Utf32BE:
        return decodeUtf32(raw, true);
    case Charset::Latin1:
        return QString::fromLatin1(raw);
    case Charset::Utf8Bom:
    case Charset::Utf8: {
        QByteArray d = raw;
        if (d.startsWith("\xEF\xBB\xBF"))
            d.remove(0, 3);
        return QString::fromUtf8(d);
    }
    case Charset::Ascii:
        return QString::fromLatin1(raw);
    }
    return QString::fromUtf8(raw);
}

QByteArray EncodingDetector::decodeToUtf8(const QByteArray& raw, const EncodingInfo& enc)
{
    if (enc.charset == Charset::Utf8 || enc.charset == Charset::Utf8Bom
        || enc.charset == Charset::Ascii) {
        QByteArray d = raw;
        if (d.startsWith("\xEF\xBB\xBF"))
            d.remove(0, 3);
        return d;
    }
    return decode(raw, enc).toUtf8();
}

QByteArray EncodingDetector::encode(const QString& text, const EncodingInfo& enc,
                                    bool* representable)
{
    if (representable)
        *representable = true;
    switch (enc.charset) {
    case Charset::Utf16LE:
        return encodeUtf16(text, false, enc.bom);
    case Charset::Utf16BE:
        return encodeUtf16(text, true, enc.bom);
    case Charset::Utf32LE:
        return encodeUtf32(text, false, enc.bom);
    case Charset::Utf32BE:
        return encodeUtf32(text, true, enc.bom);
    case Charset::Latin1:
        if (QString::fromLatin1(text.toLatin1()) != text) {
            if (representable)
                *representable = false;
            return {};
        }
        return text.toLatin1();
    case Charset::Ascii: {
        for (const QChar c : text) {
            if (c.unicode() > 0x7f) {
                if (representable)
                    *representable = false;
                return {};
            }
        }
        return text.toLatin1();
    }
    case Charset::Utf8Bom: {
        QByteArray out = text.toUtf8();
        out.prepend("\xEF\xBB\xBF");
        return out;
    }
    case Charset::Utf8:
        return text.toUtf8();
    }
    return text.toUtf8();
}

QByteArray EncodingDetector::encodeFromUtf8(const QByteArray& utf8, const EncodingInfo& enc,
                                            bool* representable)
{
    if (representable)
        *representable = true;
    if (enc.charset == Charset::Utf8)
        return utf8;
    if (enc.charset == Charset::Utf8Bom) {
        QByteArray out = utf8;
        if (!out.startsWith("\xEF\xBB\xBF"))
            out.prepend("\xEF\xBB\xBF");
        return out;
    }
    return encode(QString::fromUtf8(utf8), enc, representable);
}

Charset EncodingDetector::charsetFromName(const QString& name)
{
    const QString n = name.trimmed().toLower();
    if (n == "utf-8" || n == "utf8" || n == "utf-8-bom")
        return n.contains("bom") ? Charset::Utf8Bom : Charset::Utf8;
    if (n == "ascii" || n == "ansi" || n == "us-ascii")
        return Charset::Ascii;
    if (n == "utf-16" || n == "utf-16le" || n == "utf16le")
        return Charset::Utf16LE;
    if (n == "utf-16be" || n == "utf16be")
        return Charset::Utf16BE;
    if (n == "utf-32" || n == "utf-32le" || n == "utf32le")
        return Charset::Utf32LE;
    if (n == "utf-32be" || n == "utf32be")
        return Charset::Utf32BE;
    if (n == "latin1" || n == "iso-8859-1" || n == "iso8859-1" || n == "latin-1")
        return Charset::Latin1;
    return Charset::Utf8;
}

QString EncodingDetector::charsetName(Charset c)
{
    switch (c) {
    case Charset::Ascii:
        return QStringLiteral("ASCII");
    case Charset::Utf8:
        return QStringLiteral("UTF-8");
    case Charset::Utf8Bom:
        return QStringLiteral("UTF-8-BOM");
    case Charset::Utf16LE:
        return QStringLiteral("UTF-16LE");
    case Charset::Utf16BE:
        return QStringLiteral("UTF-16BE");
    case Charset::Utf32LE:
        return QStringLiteral("UTF-32LE");
    case Charset::Utf32BE:
        return QStringLiteral("UTF-32BE");
    case Charset::Latin1:
        return QStringLiteral("ISO-8859-1");
    }
    return QStringLiteral("UTF-8");
}

QString EncodingDetector::eolName(Eol eol)
{
    switch (eol) {
    case Eol::Lf:
        return QStringLiteral("LF");
    case Eol::CrLf:
        return QStringLiteral("CRLF");
    case Eol::Cr:
        return QStringLiteral("CR");
    case Eol::Mixed:
        return QStringLiteral("Mixed");
    case Eol::None:
        break;
    }
    return QStringLiteral("None");
}

QByteArray EncodingDetector::eolBytes(Eol eol)
{
    switch (eol) {
    case Eol::Lf:
        return QByteArrayLiteral("\n");
    case Eol::CrLf:
        return QByteArrayLiteral("\r\n");
    case Eol::Cr:
        return QByteArrayLiteral("\r");
    case Eol::Mixed:
    case Eol::None:
        break;
    }
    return QByteArrayLiteral("\n");
}

} // namespace PadForge
