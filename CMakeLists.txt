cmake_minimum_required(VERSION 3.15)
project(libsamp LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE PLUGIN_SOURCES 
    "src/*.cpp"
    "src/*.c"
    "src/*.h"
    "src/*.hpp"
)

file(GLOB_RECURSE MINHOOK_SOURCES 
    "deps/minhook/src/*.c"
    "deps/minhook/src/*.h"
)

add_library(${PROJECT_NAME} SHARED 
    ${PLUGIN_SOURCES}
    ${MINHOOK_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE 
    "src"
    "deps/minhook/include"
    "deps/minhook/src"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    SUFFIX ".asi"
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE 
        /MT 
        /W3 
        /source-charset:utf-8 
        /execution-charset:windows-1251
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        _CRT_SECURE_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
    )
endif()


