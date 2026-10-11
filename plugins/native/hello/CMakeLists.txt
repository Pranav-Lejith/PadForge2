cmake_minimum_required(VERSION 3.21)
project(padforge_hello_plugin LANGUAGES C)

add_library(padforge_hello MODULE hello_plugin.c)

# PluginApi.h lives in the PadForge core include tree.
target_include_directories(padforge_hello PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/core")

set_target_properties(padforge_hello PROPERTIES PREFIX "" OUTPUT_NAME "hello")

if(UNIX)
    set_target_properties(padforge_hello PROPERTIES SUFFIX ".so")
endif()
