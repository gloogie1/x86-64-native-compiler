#include "compiler/source/source_file.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>


namespace compiler{

SourceFile::SourceFile(std::filesystem::path path, std::string contents)
    : path_(std::move(path)),
    contents_(std::move(contents)){
        line_starts_.push_back(0);
        for(std::size_t i=0; contents_.size()>i;i++){
            if(contents_[i]=='\n'){
                line_starts_.push_back(i+1);
            }
        }
}

const std::filesystem::path& SourceFile::path() const noexcept{
    return path_;
}

std::string_view SourceFile::contents() const noexcept{
    return contents_;
}

std::size_t SourceFile::size() const noexcept{
    return contents_.size();
}

SourcePosition SourceFile::position(SourceOffset offset) const {
    if (offset > contents_.size()) {
        throw std::out_of_range{"source offset is out of range"};
    }

    const auto after_line = std::upper_bound(
        line_starts_.begin(),
        line_starts_.end(),
        offset
    );

    const auto line_start = std::prev(after_line);

    const auto zero_based_line = std::distance(line_starts_.begin(), line_start);

    const std::size_t line = static_cast<std::size_t>(zero_based_line) + 1;

    const std::size_t column = offset - *line_start + 1;

    return SourcePosition{line, column};
}

SourceLoadResult load_source_file(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);

    if (!file) {
        return SourceLoadError{path, "Could not open the file."};
    }

    const auto end_position = file.tellg();

    if (end_position == std::streampos{-1}) {
        return SourceLoadError{path, "Could not determine file size."};
    }

    const auto file_size = static_cast<std::size_t>(end_position);

    file.seekg(0, std::ios::beg);

    if (!file) {
        return SourceLoadError{path, "Could not seek to beginning of file."};
    }

    std::string buffer(file_size, '\0');

    if (file_size > 0) {
        const auto read_size = static_cast<std::streamsize>(file_size);

        if (!file.read(buffer.data(), read_size)) {
            return SourceLoadError{path, "Error reading the file."};
        }
    }

    return SourceFile{path, std::move(buffer)};
}

}