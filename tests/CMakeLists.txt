find_package(Qt6 REQUIRED COMPONENTS Test)

function(padforge_add_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE padforge_core Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;PADFORGE_TESTING=1")
endfunction()

padforge_add_test(test_encoding)
padforge_add_test(test_search)
padforge_add_test(test_session)
padforge_add_test(test_macro)
padforge_add_test(test_textops)

if(PADFORGE_ENABLE_LUA)
    padforge_add_test(test_plugin)
    target_compile_definitions(test_plugin PRIVATE
        PADFORGE_TEST_PLUGIN_DIR="${CMAKE_SOURCE_DIR}/plugins/lua")
endif()

# Headless UI smoke test (only when the GUI target is available).
if(TARGET padforge_gui)
    add_executable(test_smoke test_smoke.cpp)
    target_link_libraries(test_smoke PRIVATE padforge_gui padforge_core Qt6::Test Qt6::Widgets)
    add_test(NAME test_smoke COMMAND test_smoke)
    set_tests_properties(test_smoke PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;PADFORGE_TESTING=1")
endif()
