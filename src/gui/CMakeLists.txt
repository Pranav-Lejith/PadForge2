find_package(Qt6 REQUIRED COMPONENTS PrintSupport)

add_library(padforge_gui STATIC
    MainWindow.cpp
    MainWindow.h
    EditorWidget.cpp
    EditorWidget.h
    FindReplaceDialog.cpp
    FindReplaceDialog.h
    FindInFilesPanel.cpp
    FindInFilesPanel.h
    PreferencesDialog.cpp
    PreferencesDialog.h
    ColumnEditorDialog.cpp
    ColumnEditorDialog.h
    GotoLineDialog.cpp
    GotoLineDialog.h
    DocumentSummaryDialog.cpp
    DocumentSummaryDialog.h
    RunDialog.cpp
    RunDialog.h
    ExplorerPanel.cpp
    ExplorerPanel.h
    DocumentMap.cpp
    DocumentMap.h
    FloatingWindow.cpp
    FloatingWindow.h
    ConsolePanel.cpp
    ConsolePanel.h
    FunctionListPanel.cpp
    FunctionListPanel.h
    CharacterMapPanel.cpp
    CharacterMapPanel.h
    CompareDocumentsDialog.cpp
    CompareDocumentsDialog.h
    SearchResultsPanel.cpp
    SearchResultsPanel.h)

target_include_directories(padforge_gui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(padforge_gui PUBLIC padforge_core PadForge::scintilla PadForge::lexilla
    Qt6::Widgets Qt6::PrintSupport)
target_compile_definitions(padforge_gui PUBLIC SCINTILLA_QT=1)

add_executable(padforge main.cpp)
target_link_libraries(padforge PRIVATE padforge_gui)

if(APPLE)
    set_target_properties(padforge PROPERTIES
        OUTPUT_NAME PadForge
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER org.padforge.PadForge
        MACOSX_BUNDLE_BUNDLE_NAME PadForge
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
endif()

if(WIN32)
    set_target_properties(padforge PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if(APPLE)
    install(TARGETS padforge BUNDLE DESTINATION .)
else()
    install(TARGETS padforge RUNTIME DESTINATION bin)
endif()
