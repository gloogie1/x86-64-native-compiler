cmake_minimum_required(VERSION 3.20)

project(
    x86_64_native_compiler
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(
    compiler_core
    src/source/source_file.cpp
    src/lexer/token.cpp
    src/lexer/lexer.cpp
)

target_include_directories(
    compiler_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_options(
    compiler_core
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
)

add_executable(
    x86-64-native-compiler
    src/driver/main.cpp
)

target_link_libraries(
    x86-64-native-compiler
    PRIVATE
        compiler_core
)

target_compile_options(
    x86-64-native-compiler
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
)

include(CTest)

if(BUILD_TESTING)

    # Existing SourceFile tests
    add_executable(
        source_file_tests
        tests/source_file_tests.cpp
    )

    target_link_libraries(
        source_file_tests
        PRIVATE
            compiler_core
    )

    target_compile_options(
        source_file_tests
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
    )

    add_test(
        NAME source_file_tests
        COMMAND source_file_tests
    )

    # GoogleTest
    include(FetchContent)

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
    )

    FetchContent_MakeAvailable(googletest)

    # Lexer tests
    add_executable(
        lexer_tests
        tests/lexer_tests.cpp
    )

    target_link_libraries(
        lexer_tests
        PRIVATE
            compiler_core
            GTest::gtest_main
    )

    target_compile_options(
        lexer_tests
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
    )

    include(GoogleTest)
    gtest_discover_tests(lexer_tests)

endif()