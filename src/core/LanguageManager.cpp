#include "LanguageManager.h"

#include <QFileInfo>

namespace PadForge {

namespace {

const char* kCKeywords =
    "auto break case char const continue default do double else enum extern float for goto if int long "
    "register return short signed sizeof static struct switch typedef union unsigned void volatile while";

const char* kCppKeywords =
    "alignas alignof and asm auto bool break case catch char class const constexpr const_cast continue "
    "decltype default delete do double dynamic_cast else enum explicit export extern false float for friend "
    "goto if inline int long mutable namespace new noexcept nullptr operator private protected public "
    "register reinterpret_cast return short signed sizeof static static_assert static_cast struct switch "
    "template this throw true try typedef typeid typename union unsigned using virtual void volatile wchar_t while";

const char* kJavaKeywords =
    "abstract assert boolean break byte case catch char class const continue default do double else enum "
    "extends final finally float for goto if implements import instanceof int interface long native new "
    "package private protected public return short static strictfp super switch synchronized this throw "
    "throws transient try void volatile while true false null";

const char* kCSharpKeywords =
    "abstract as base bool break byte case catch char checked class const continue decimal default "
    "delegate do double else enum event explicit extern false finally fixed float for foreach goto if "
    "implicit in int interface internal is lock long namespace new null object operator out override "
    "params private protected public readonly ref return sbyte sealed short sizeof stackalloc static "
    "string struct switch this throw true try typeof uint ulong unchecked unsafe ushort using virtual void volatile while";

const char* kPythonKeywords =
    "and as assert async await break class continue def del elif else except False finally for from "
    "global if import in is lambda None nonlocal not or pass raise return True try while with yield";

const char* kJsKeywords =
    "abstract arguments await boolean break byte case catch char class const continue debugger default "
    "delete do double else enum eval export extends false final finally float for function goto if "
    "implements import in instanceof int interface let long native new null package private protected "
    "public return short static super switch synchronized this throw throws transient true try typeof "
    "var void volatile while with yield async of";

const char* kLuaKeywords =
    "and break do else elseif end false for function goto if in local nil not or repeat return then "
    "true until while";

const char* kSqlKeywords =
    "ADD ALL ALTER AND ANY AS ASC BACKUP BETWEEN BY CASE CHECK COLUMN CONSTRAINT CREATE DATABASE DEFAULT "
    "DELETE DESC DISTINCT DROP EXEC FOREIGN FROM FULL GROUP HAVING IN INDEX INNER INSERT INTO IS JOIN KEY "
    "LEFT LIKE LIMIT NOT NULL OR ORDER OUTER PRIMARY PROCEDURE RIGHT ROW SELECT SET TABLE TOP TRUNCATE "
    "UNION UNIQUE UPDATE VALUES VIEW WHERE";

const char* kPhpKeywords =
    "abstract and array as break callable case catch class clone const continue declare default do echo "
    "else elseif empty enddeclare endfor endforeach endif endswitch endwhile enum extends final finally fn "
    "for foreach function global goto if implements include include_once instanceof insteadof interface "
    "isset list match namespace new or print private protected public readonly require require_once return "
    "static switch throw trait try unset use var while xor yield";

const char* kBashKeywords =
    "if then else elif fi case esac for select while until do done in function time coproc break continue "
    "return exit export local readonly declare typeset unset shift source alias";

const char* kGoKeywords =
    "break default func interface select case defer go map struct chan else goto package switch const "
    "fallthrough if range type continue for import return var";

const char* kRustKeywords =
    "as async await break const continue crate dyn else enum extern false fn for if impl in let loop "
    "match mod move mut pub ref return self Self static struct super trait true type unsafe use where while";

const QList<Language>& build()
{
    static QList<Language> langs;
    if (!langs.isEmpty())
        return langs;

    auto add = [&](const char* id, const char* name, const char* lexer, const QStringList& ext,
                   const QStringList& shebang = {}, const char* keywords = "") {
        langs.append(Language{QString::fromLatin1(id), QString::fromLatin1(name),
                              QString::fromLatin1(lexer), ext, shebang,
                              QString::fromLatin1(keywords)});
    };

    add("text", "Normal Text", "null", {"txt", "log", "text"});
    add("c", "C", "cpp", {"c"}, {}, kCKeywords);
    add("cpp", "C++", "cpp", {"cpp", "cc", "cxx", "c++", "hpp", "hh", "hxx", "h", "ino"}, {}, kCppKeywords);
    add("objectivec", "Objective-C", "cpp", {"m", "mm"}, {}, kCppKeywords);
    add("csharp", "C#", "cs", {"cs"}, {}, kCSharpKeywords);
    add("java", "Java", "cpp", {"java"}, {}, kJavaKeywords);
    add("python", "Python", "python", {"py", "pyw", "pyi"},
        {"python", "python3"}, kPythonKeywords);
    add("javascript", "JavaScript", "javascript", {"js", "mjs", "cjs", "jsx"}, {"node"}, kJsKeywords);
    add("typescript", "TypeScript", "cpp", {"ts", "tsx"}, {}, kCppKeywords);
    add("html", "HTML", "hypertext", {"html", "htm", "xhtml", "shtml"});
    add("xml", "XML", "xml", {"xml", "xsd", "xsl", "xslt", "svg", "plist"});
    add("css", "CSS", "css", {"css", "scss", "less"});
    add("json", "JSON", "json", {"json", "jsonc", "geojson"});
    add("yaml", "YAML", "yaml", {"yaml", "yml"});
    add("markdown", "Markdown", "markdown", {"md", "markdown", "mkd"});
    add("sql", "SQL", "sql", {"sql"}, {}, kSqlKeywords);
    add("shell", "Shell", "bash", {"sh", "bash", "zsh", "ksh"}, {"sh", "bash", "zsh"}, kBashKeywords);
    add("perl", "Perl", "perl", {"pl", "pm", "t"}, {"perl"});
    add("ruby", "Ruby", "ruby", {"rb", "rake", "gemspec"}, {"ruby"});
    add("php", "PHP", "php", {"php", "php3", "php4", "php5", "phtml"}, {}, kPhpKeywords);
    add("lua", "Lua", "lua", {"lua"}, {"lua"}, kLuaKeywords);
    add("makefile", "Makefile", "makefile", {"mk", "make", "mak", "makefile"});
    add("cmake", "CMake", "cmake", {"cmake"});
    add("diff", "Diff", "diff", {"diff", "patch"});
    add("latex", "LaTeX", "latex", {"tex", "latex", "sty", "cls"});
    add("batch", "Batch", "batch", {"bat", "cmd"});
    add("pascal", "Pascal", "pascal", {"pas", "pp", "dpr"});
    add("fortran", "Fortran", "fortran", {"f", "for", "f77", "f90", "f95", "f03"});
    add("haskell", "Haskell", "haskell", {"hs", "lhs"});
    add("erlang", "Erlang", "erlang", {"erl", "hrl"});
    add("lisp", "Lisp", "lisp", {"lisp", "lsp", "cl"});
    add("scheme", "Scheme", "lisp", {"scm", "ss", "rkt"});
    add("tcl", "Tcl", "tcl", {"tcl", "tk"}, {"tclsh", "wish"});
    add("vb", "Visual Basic", "vb", {"vb", "bas", "frm"});
    add("ada", "Ada", "ada", {"adb", "ads", "ada"});
    add("asm", "Assembly", "asm", {"asm", "s", "nasm"});
    add("d", "D", "d", {"d", "di"});
    add("matlab", "MATLAB", "matlab", {"m"});
    add("r", "R", "r", {"r", "R"});
    add("swift", "Swift", "cpp", {"swift"}, {}, kCppKeywords);
    add("go", "Go", "cpp", {"go"}, {}, kGoKeywords);
    add("rust", "Rust", "rust", {"rs"}, {}, kRustKeywords);
    add("kotlin", "Kotlin", "kotlin", {"kt", "kts"});
    add("julia", "Julia", "julia", {"jl"});
    add("ini", "INI / Properties", "props", {"ini", "cfg", "conf", "properties", "desktop"});
    add("nsis", "NSIS", "nsis", {"nsi", "nsh"});
    add("cobol", "COBOL", "cobol", {"cob", "cbl", "cpy"});
    add("vhdl", "VHDL", "vhdl", {"vhd", "vhdl"});
    add("verilog", "Verilog", "verilog", {"v", "vh", "sv"});
    add("powershell", "PowerShell", "powershell", {"ps1", "psm1", "psd1"}, {"pwsh", "powershell"});
    add("fsharp", "F#", "fsharp", {"fs", "fsi", "fsx"});
    add("dockerfile", "Dockerfile", "bash", {"dockerfile"}, {}, kBashKeywords);
    add("groovy", "Groovy", "cpp", {"groovy", "gradle"}, {}, kJavaKeywords);
    add("toml", "TOML", "props", {"toml"});

    return langs;
}

} // namespace

const QList<Language>& LanguageManager::languages()
{
    return build();
}

const Language* LanguageManager::byId(const QString& id)
{
    for (const Language& l : languages())
        if (l.id.compare(id, Qt::CaseInsensitive) == 0)
            return &l;
    return nullptr;
}

const Language* LanguageManager::forFile(const QString& path, const QString& firstLine)
{
    const QString fileName = QFileInfo(path).fileName().toLower();
    const QString suffix = QFileInfo(path).suffix().toLower();

    if (!fileName.isEmpty()) {
        for (const Language& l : languages()) {
            for (const QString& ext : l.extensions)
                if (fileName == ext.toLower())
                    return &l;
        }
        if (fileName == "makefile" || fileName == "gnumakefile")
            return byId("makefile");
        if (fileName == "cmakelists.txt")
            return byId("cmake");
        if (fileName.startsWith("dockerfile"))
            return byId("dockerfile");
    }

    if (!suffix.isEmpty()) {
        for (const Language& l : languages()) {
            if (l.extensions.contains(suffix, Qt::CaseInsensitive))
                return &l;
        }
    }

    if (!firstLine.isEmpty()) {
        const QString line = firstLine.trimmed().toLower();
        if (line.startsWith("#!")) {
            for (const Language& l : languages()) {
                for (const QString& token : l.shebangs)
                    if (line.contains(token))
                        return &l;
            }
        }
    }

    return plainText();
}

const Language* LanguageManager::plainText()
{
    static const Language text{QStringLiteral("text"), QStringLiteral("Normal Text"),
                               QStringLiteral("null"), {"txt"}, {}, {}};
    return &text;
}

QStringList LanguageManager::displayNames()
{
    QStringList names;
    for (const Language& l : languages())
        names.append(l.name);
    return names;
}

int LanguageManager::count()
{
    return int(languages().size());
}

} // namespace PadForge
