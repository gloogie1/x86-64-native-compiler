#include "compiler/source/source_file.hpp"

#include <iostream>
#include <cassert>
#include <cstddef>
#include <stdexcept>

void expect_position(
    const compiler::SourceFile& source,
    std::size_t offset,
    std::size_t expected_line,
    std::size_t expected_column
) {
    const compiler::SourcePosition actual = source.position(offset);

    if (actual.line != expected_line ||
    actual.column != expected_column) {
    std::cerr
        << "position(" << offset << ") mismatch\n"
        << "expected: " << expected_line << ':' << expected_column << '\n'
        << "actual:   " << actual.line << ':' << actual.column << '\n';
    }
    assert(actual.line == expected_line);
    assert(actual.column == expected_column);
}

void test_empty_file() {
    std::cerr << "Running test_empty_file\n";
    const compiler::SourceFile source{"empty.xc",""};

    assert(source.size() == 0);
    expect_position(source, 0, 1, 1);
}

void test_single_line() {
    std::cerr << "Running test_single_line\n";
    const compiler::SourceFile source("single.xc", "abc");

    expect_position(source, 0, 1, 1);
    expect_position(source, 1, 1, 2);
    expect_position(source, 2, 1, 3);
    expect_position(source, 3, 1, 4);
}

void test_multiple_lines(){
    std::cerr << "Running test_multiple_lines\n";
    const compiler::SourceFile source{
        "multiple.xc",
        "abc\nde"
    };

    expect_position(source, 0, 1, 1);
    expect_position(source, 3, 1, 4); // newline
    expect_position(source, 4, 2, 1); // d
    expect_position(source, 5, 2, 2); // e
    expect_position(source, 6, 2, 3); // EOF
}

void test_trailing_newline() {
    std::cerr << "Running test_trailing_newline\n";
    const compiler::SourceFile source{
        "trailing.xc",
        "abc\n"
    };

    expect_position(source, 3, 1, 4); // newline
    expect_position(source, 4, 2, 1); // EOF on empty second line
}

void test_invalid_offset() {
    std::cerr << "Running test_invalid_offset\n";
    const compiler::SourceFile source{"example.xc", "abc"};

    bool threw_out_of_range = false;
    
    try {
        static_cast<void>(source.position(source.size() + 1));
    } catch (const std::out_of_range&) {
        threw_out_of_range = true;
    }

    assert(threw_out_of_range);
}

void test_basic_accessors() {
    std::cerr << "Running test_basic_accessors\n";
    const compiler::SourceFile source{
        "example.xc",
        "abc"
    };

    assert(source.path() == "example.xc");
    assert(source.contents() == "abc");
    assert(source.size() == 3);
}


int main() {
    test_empty_file();
    test_single_line();
    test_multiple_lines();
    test_trailing_newline();
    test_invalid_offset();
    test_basic_accessors();

    return 0;
}
