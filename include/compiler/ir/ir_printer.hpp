#pragma once

#include "compiler/ir/ir.hpp"
#include <string>

namespace compiler::ir {

std::string dump_ir(const IRProgram& program);

} // namespace compiler::ir