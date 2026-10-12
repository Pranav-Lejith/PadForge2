cmake_minimum_required(VERSION 3.21)

project(PadForge
    VERSION 0.1.0
    DESCRIPTION "A fast native Linux text editor with a Notepad++-style workflow"
    HOMEPAGE_URL "https://github.com/Pranav-Lejith/PadForge2"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(PADFORGE_BUILD_TESTS "Build the PadForge test suite" ON)
option(PADFORGE_ENABLE_LUA "Enable the Lua plugin runtime" ON)
option(PADFORGE_USE_SYSTEM_SCINTILLA
    "Use a system Scintilla/QScintilla instead of the pinned sources" OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

if(PADFORGE_USE_SYSTEM_SCINTILLA)
    message(STATUS "PadForge: using system Qt6 only, expecting a system Scintilla provider")
else()
    include(Scintilla)
endif()

add_subdirectory(src/core)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/CMakeLists.txt" AND TARGET PadForge::scintilla)
    add_subdirectory(src/gui)
else()
    message(STATUS "PadForge: GUI not built (needs Scintilla) - building core library only")
endif()

if(PADFORGE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ---- Desktop integration --------------------------------------------------
if(UNIX AND TARGET padforge)
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/org.padforge.PadForge.desktop"
        DESTINATION share/applications)
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/org.padforge.PadForge.svg"
        DESTINATION share/icons/hicolor/scalable/apps)
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/org.padforge.PadForge.metainfo.xml"
        DESTINATION share/metainfo)
endif()

# ---- Packaging metadata ---------------------------------------------------
set(CPACK_PACKAGE_NAME "PadForge")
set(CPACK_PACKAGE_VENDOR "PadForge contributors")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_CONTACT "PadForge contributors")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_HOMEPAGE_URL "${PROJECT_HOMEPAGE_URL}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_GENERATOR "TGZ")
include(CPack)
