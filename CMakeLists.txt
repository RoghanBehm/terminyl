cmake_minimum_required(VERSION 3.16)
project(terminyl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_CLANG_TIDY "Enable clang-tidy checks during compilation" OFF)

if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        message(STATUS "clang-tidy enabled for build: ${CLANG_TIDY_EXE}")
        set(CMAKE_CXX_CLANG_TIDY 
            ${CLANG_TIDY_EXE};
            --warnings-as-errors=*;
        )
    endif()
endif()

set(SOURCES
    src/main.cpp
    src/lexer.cpp
    src/token.cpp
    src/parser.cpp
    src/io.cpp
    src/emitter.cpp
    src/document.cpp
    src/lowerer.cpp
    src/error.cpp
    src/environment.cpp
    src/value.cpp
)

add_library(core STATIC
    ${SOURCES}
)

target_include_directories(core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(terminyl
    src/main.cpp
)

target_link_libraries(terminyl
    PRIVATE core
)

install(TARGETS terminyl RUNTIME DESTINATION bin)