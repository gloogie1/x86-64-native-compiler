#include "compiler/ir/ir_printer.hpp"

#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

namespace compiler::ir {

// Formatting helpers for enums
std::string_view binary_op_name(BinaryOp op) {
    switch (op) {
        case BinaryOp::Add:          return "add";
        case BinaryOp::Subtract:     return "sub";
        case BinaryOp::Multiply:     return "mul";
        case BinaryOp::Divide:       return "div";
        case BinaryOp::Remainder:    return "rem";
        case BinaryOp::Equal:        return "eq";
        case BinaryOp::NotEqual:     return "ne";
        case BinaryOp::Less:         return "lt";
        case BinaryOp::LessEqual:    return "le";
        case BinaryOp::Greater:      return "gt";
        case BinaryOp::GreaterEqual: return "ge";
    }

    return "<unknown>";
}

std::string_view ir_type_name(IRType type) {
    switch (type) {
        case IRType::I64:  return "i64";
        case IRType::Bool: return "bool";
        case IRType::Void: return "void";
    }

    return "<unknown>";
}

// Instruction printing helper
void print_instruction(std::ostringstream& out, const Instruction& instruction) {
    std::visit([&](const auto& inst){
        using T = std::decay_t<decltype(inst)>;
        
        if constexpr (std::is_same_v<T, ConstI64>) {
            out << "    %v" << inst.result.value
                << " = const.i64 " << inst.value << '\n';
        }
        else if constexpr (std::is_same_v<T, ConstBool>) {
            out << "    %v" << inst.result.value
                << " = const.bool "
                << (inst.value ? "true" : "false") << '\n';
        }
        else if constexpr (std::is_same_v<T, LoadLocal>) {
            out << "    %v" << inst.result.value
                << " = load $l" << inst.local.value << '\n';
        }
        else if constexpr (std::is_same_v<T, StoreLocal>) {
            out << "    store $l" << inst.local.value
                << ", %v" << inst.value.value << '\n';
        }
        else if constexpr (std::is_same_v<T, Unary>) {
            switch(inst.op){
                case UnaryOp::Negate:
                    out << "    %v" << inst.result.value
                        << " = neg %v" << inst.operand.value << '\n';
                    break;
                case UnaryOp::LogicalNot:
                    out << "    %v" << inst.result.value
                        << " = not %v" << inst.operand.value << '\n';
                    break;
            }
        }
        else if constexpr (std::is_same_v<T, Binary>) {
            out << "    %v" << inst.result.value
                << " = " << binary_op_name(inst.op)
                << " %v" << inst.left.value
                << ", %v" << inst.right.value << '\n';
        }
        else if constexpr (std::is_same_v<T, Call>) {
            out << "    ";

            if (inst.result.has_value()) {
                out << "%v" << inst.result->value << " = ";
            }

            out << "call @f" << inst.callee.value << "(";

            for (std::size_t i = 0; i < inst.arguments.size(); ++i) {
                if (i > 0) {
                    out << ", ";
                }

                out << "%v" << inst.arguments[i].value;
            }

            out << ")\n";
        }
    }, instruction);
}

// Terminator printing helper
void print_terminator(std::ostringstream& out, const Terminator& terminator) {
    std::visit([&](const auto& term) {
        using T = std::decay_t<decltype(term)>;

        if constexpr (std::is_same_v<T, BranchTerminator>) {
            out << "    br bb" << term.target.value << '\n';
        }
        else if constexpr (std::is_same_v<T, CondBranchTerminator>) {
            out << "    br_if %v" << term.condition.value
                << ", bb" << term.true_target.value
                << ", bb" << term.false_target.value << '\n';
        }
        else if constexpr (std::is_same_v<T, ReturnTerminator>) {
            out << "    ret";

            if (term.value.has_value()) {
                out << " %v" << term.value->value;
            }

            out << '\n';
        }
    }, terminator);
}

std::string dump_ir(const IRProgram& program) {
    std::ostringstream out;

    for (std::size_t f = 0; f < program.functions.size(); ++f) {
        const IRFunction& function = program.functions[f];

        // Function header
        out << "fn @" << function.name << "(";

        for (std::size_t i = 0; i < function.parameter_types.size(); ++i) {
            if (i > 0) {
                out << ", ";
            }

            out << "$l" << function.parameter_locals[i].value
                << ": " << ir_type_name(function.parameter_types[i]);
        }

        out << ") -> " << ir_type_name(function.return_type) << " {\n";

        // Basic blocks
        for (const BasicBlock& block : function.blocks) {
            out << "bb" << block.id.value << ":\n";

            // Instructions
            for (const Instruction& instruction : block.instructions) {
                print_instruction(out, instruction);
            }

            // Terminator
            if (block.terminator.has_value()) {
                print_terminator(out, *block.terminator);
            } else {
                out << "    <unterminated>\n";
            }
        }

        out << "}\n";

        // Separate functions with one blank line
        if (f + 1 < program.functions.size()) {
            out << '\n';
        }
    }

    return out.str();
}

} // namespace compiler::ir