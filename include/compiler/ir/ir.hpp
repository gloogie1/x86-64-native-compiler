#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <variant>
#include <optional>

namespace compiler::ir {

enum class IRType {
    I64,
    Bool,
    Void
};

enum class UnaryOp {
    Negate,
    LogicalNot
};

enum class BinaryOp {
    Add,
    Subtract,
    Multiply,
    Divide,
    Remainder,
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual
};

struct ValueId {
    std::uint32_t value;
    bool operator==(const ValueId&) const = default;
};

struct LocalId {
    std::uint32_t value;
    bool operator==(const LocalId&) const = default;
};

struct BlockId {
    std::uint32_t value;
    bool operator==(const BlockId&) const = default;
};

struct FunctionId {
    std::uint32_t value;
    bool operator==(const FunctionId&) const = default;
};

struct Local {
    LocalId id;
    IRType type;
    std::string name;
};

struct ConstI64 {
    ValueId result;
    std::int64_t value;
};

struct ConstBool {
    ValueId result;
    bool value;
};

struct LoadLocal {
    ValueId result;
    LocalId local;
};

struct StoreLocal {
    LocalId local;
    ValueId value;
};

struct Unary {
    ValueId result;
    UnaryOp op;
    ValueId operand;
};

struct Binary {
    ValueId result;
    BinaryOp op;
    ValueId left;
    ValueId right;
};

struct Call {
    std::optional<ValueId> result;
    FunctionId callee;
    std::vector<ValueId> arguments;
};

using Instruction = std::variant<
    ConstI64,
    ConstBool,
    LoadLocal,
    StoreLocal,
    Unary,
    Binary,
    Call
>;

struct BranchTerminator {
    BlockId target;
};

struct CondBranchTerminator {
    ValueId condition;
    BlockId true_target;
    BlockId false_target;
};

struct ReturnTerminator {
    std::optional<ValueId> value;
};

using Terminator = std::variant<
    BranchTerminator,
    CondBranchTerminator,
    ReturnTerminator
>;

struct BasicBlock {
    BlockId id;
    std::string debug_name;
    std::vector<Instruction> instructions;
    std::optional<Terminator> terminator;
};

struct IRFunction {
    FunctionId id;
    std::string name;
    IRType return_type;
    std::vector<IRType> parameter_types;
    std::vector<LocalId> parameter_locals;
    std::vector<Local> locals;
    std::vector<BasicBlock> blocks;
};

struct IRProgram {
    std::vector<IRFunction> functions;
};

} // namespace compiler::ir