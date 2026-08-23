#include "compiler/source/source_file.hpp"

#include <iostream>
#include <variant>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "usage: x86-64-native-compiler <source-file>\n";
        return 1;
    }

    compiler::SourceLoadResult result = compiler::load_source_file(argv[1]);

    if (const auto* error = std::get_if<compiler::SourceLoadError>(&result)) {
        std::cerr << error->path.string() << ": " << error->message<< '\n';
        return 1;
    }

    return 0;
}