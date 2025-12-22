cmake_minimum_required(VERSION 3.16)

project(typscii LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SOURCES
    src/main.cpp
    src/lexer.cpp
    src/token.cpp
)

add_library(core STATIC
    ${SOURCES}
)

target_include_directories(core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(typscii
    src/main.cpp
)

target_link_libraries(typscii
    PRIVATE core
)

add_custom_target(clang-tidy
    COMMAND clang-tidy
        -p ${CMAKE_BINARY_DIR}
        ${SOURCES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-tidy on typscii sources"
)
