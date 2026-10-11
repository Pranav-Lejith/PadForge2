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
    RunDialog.h)

target_include_directories(padforge_gui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(padforge_gui PUBLIC padforge_core PadForge::scintilla PadForge::lexilla Qt6::Widgets)
target_compile_definitions(padforge_gui PUBLIC SCINTILLA_QT=1)

add_executable(padforge main.cpp)
target_link_libraries(padforge PRIVATE padforge_gui)

if(WIN32)
    set_target_properties(padforge PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

install(TARGETS padforge RUNTIME DESTINATION bin)
