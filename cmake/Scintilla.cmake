# Builds Scintilla (with the in-tree Qt ScintillaEdit binding) and Lexilla
# from pinned upstream sources, exposing them as PadForge::scintilla and
# PadForge::lexilla.  Upstream ships no CMakeLists.txt, so we fetch the source
# archives and provide our own targets (the pattern used by other Qt editors).

include(FetchContent)
include_guard(GLOBAL)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Core5Compat)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(PADFORGE_SCINTILLA_VERSION "5.6.7")
set(PADFORGE_LEXILLA_VERSION "5.5.4")
set(PADFORGE_SCINTILLA_URL "https://downloads.sourceforge.net/project/scintilla/scintilla/5.6.7/scintilla567.tgz")
set(PADFORGE_SCINTILLA_SHA256 "19e4476706750056f86ca70974810e4a5b08e73477e13f7d9fae78243606f6a7")
set(PADFORGE_LEXILLA_URL "https://downloads.sourceforge.net/project/scintilla/lexilla/5.5.4/lexilla554.tgz")
set(PADFORGE_LEXILLA_SHA256 "fcd9ef74236a12359d1039827c93dbf827aa476c46f9fd6eef60bb96db1af2f6")

FetchContent_Declare(scintilla
    URL ${PADFORGE_SCINTILLA_URL}
    URL_HASH SHA256=${PADFORGE_SCINTILLA_SHA256}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

FetchContent_Declare(lexilla
    URL ${PADFORGE_LEXILLA_URL}
    URL_HASH SHA256=${PADFORGE_LEXILLA_SHA256}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

# Neither archive contains a CMakeLists.txt, so MakeAvailable only downloads and
# unpacks them; we wire the targets ourselves below.
FetchContent_MakeAvailable(scintilla lexilla)

if(NOT EXISTS "${scintilla_SOURCE_DIR}/qt/ScintillaEdit/ScintillaEdit.h")
    message(STATUS "PadForge: generating ScintillaEdit.h via WidgetGen.py")
    execute_process(
        COMMAND ${Python3_EXECUTABLE} WidgetGen.py
        WORKING_DIRECTORY "${scintilla_SOURCE_DIR}/qt/ScintillaEdit"
        RESULT_VARIABLE _padforge_widgetgen_result)
    if(NOT _padforge_widgetgen_result EQUAL 0)
        message(FATAL_ERROR "PadForge: WidgetGen.py failed (${_padforge_widgetgen_result})")
    endif()
endif()

file(GLOB PADFORGE_SCINTILLA_CORE "${scintilla_SOURCE_DIR}/src/*.cxx")

add_library(scintilla STATIC
    ${PADFORGE_SCINTILLA_CORE}
    "${scintilla_SOURCE_DIR}/qt/ScintillaEdit/ScintillaEdit.cpp"
    "${scintilla_SOURCE_DIR}/qt/ScintillaEdit/ScintillaDocument.cpp"
    "${scintilla_SOURCE_DIR}/qt/ScintillaEditBase/PlatQt.cpp"
    "${scintilla_SOURCE_DIR}/qt/ScintillaEditBase/ScintillaQt.cpp"
    "${scintilla_SOURCE_DIR}/qt/ScintillaEditBase/ScintillaEditBase.cpp")

target_include_directories(scintilla PUBLIC
    "${scintilla_SOURCE_DIR}/include"
    "${scintilla_SOURCE_DIR}/src"
    "${scintilla_SOURCE_DIR}/qt/ScintillaEdit"
    "${scintilla_SOURCE_DIR}/qt/ScintillaEditBase")

target_compile_definitions(scintilla PUBLIC
    SCINTILLA_QT=1
    MAKING_LIBRARY=1
    SCI_LEXER=1
    EXPORT_IMPORT_API=
    _CRT_SECURE_NO_DEPRECATE=1)

target_link_libraries(scintilla PUBLIC
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Core5Compat)
add_library(PadForge::scintilla ALIAS scintilla)

file(GLOB PADFORGE_LEXILLA_LEXLIB "${lexilla_SOURCE_DIR}/lexlib/*.cxx")
file(GLOB PADFORGE_LEXILLA_LEXERS "${lexilla_SOURCE_DIR}/lexers/*.cxx")

add_library(lexilla STATIC
    "${lexilla_SOURCE_DIR}/src/Lexilla.cxx"
    "${lexilla_SOURCE_DIR}/access/LexillaAccess.cxx"
    ${PADFORGE_LEXILLA_LEXLIB}
    ${PADFORGE_LEXILLA_LEXERS})

target_compile_definitions(lexilla PRIVATE LEXILLA_NO_EXPORT)
target_include_directories(lexilla PUBLIC
    "${lexilla_SOURCE_DIR}/include"
    "${lexilla_SOURCE_DIR}/access"
    "${lexilla_SOURCE_DIR}/lexlib")
target_link_libraries(lexilla PUBLIC scintilla)
if(UNIX AND NOT APPLE)
    target_link_libraries(lexilla PUBLIC dl)
endif()
add_library(PadForge::lexilla ALIAS lexilla)
