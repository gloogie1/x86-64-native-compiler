#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace compiler {

using SourceOffset = std::size_t;

struct SourceSpan {
    SourceOffset begin;
    SourceOffset end;
};

struct SourcePosition {
    std::size_t line;
    std::size_t column;
};

class SourceFile {
public:
    SourceFile(std::filesystem::path path, std::string contents);

    [[nodiscard]] const std::filesystem::path& path() const noexcept;
    [[nodiscard]] std::string_view contents() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

    [[nodiscard]] SourcePosition position(SourceOffset offset) const;

private:
    std::filesystem::path path_;
    std::string contents_;
    std::vector<SourceOffset> line_starts_;
};

struct SourceLoadError {
    std::filesystem::path path;
    std::string message;
};

using SourceLoadResult = std::variant<SourceFile, SourceLoadError>;

[[nodiscard]]
SourceLoadResult load_source_file(const std::filesystem::path& path);

}  //namespace compiler
