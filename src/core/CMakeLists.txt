add_library(padforge_core STATIC
    Backup.cpp
    Diff.cpp
    Encoding.cpp
    LanguageManager.cpp
    Macro.cpp
    Outline.cpp
    PluginHost.cpp
    SearchEngine.cpp
    Session.cpp
    Settings.cpp
    TextOps.cpp
    Theme.cpp)

target_include_directories(padforge_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(padforge_core PUBLIC Qt6::Core Qt6::Gui)

if(PADFORGE_ENABLE_LUA)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(LUA QUIET lua5.4)
        pkg_check_modules(LUA55 QUIET lua5.5)
        pkg_check_modules(LUA54 QUIET lua5.4)
        if(NOT LUA_FOUND AND LUA55_FOUND)
            set(LUA_FOUND TRUE)
            set(LUA_INCLUDE_DIRS ${LUA55_INCLUDE_DIRS})
            set(LUA_LIBRARIES ${LUA55_LIBRARIES})
            set(LUA_LIBRARY_DIRS ${LUA55_LIBRARY_DIRS})
        endif()
        if(NOT LUA_FOUND AND LUA54_FOUND)
            set(LUA_FOUND TRUE)
            set(LUA_INCLUDE_DIRS ${LUA54_INCLUDE_DIRS})
            set(LUA_LIBRARIES ${LUA54_LIBRARIES})
            set(LUA_LIBRARY_DIRS ${LUA54_LIBRARY_DIRS})
        endif()
    endif()
    if(NOT LUA_FOUND)
        find_package(Lua QUIET)
        if(LUA_FOUND)
            set(LUA_INCLUDE_DIRS ${LUA_INCLUDE_DIR})
            set(LUA_LIBRARIES ${LUA_LIBRARIES})
        endif()
    endif()

    if(LUA_FOUND)
        message(STATUS "PadForge: Lua plugin runtime enabled (${LUA_LIBRARIES})")
        target_compile_definitions(padforge_core PUBLIC PADFORGE_ENABLE_LUA=1)
        target_include_directories(padforge_core PUBLIC ${LUA_INCLUDE_DIRS})
        target_link_directories(padforge_core PUBLIC ${LUA_LIBRARY_DIRS})
        target_link_libraries(padforge_core PUBLIC ${LUA_LIBRARIES} ${LUA_LIBRARY})
    else()
        message(WARNING "PadForge: Lua not found - building without the Lua plugin runtime")
    endif()
endif()
