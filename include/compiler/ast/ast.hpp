#pragma once

#include "compiler/source/source_file.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace compiler {

enum class TypeKind {
    I64,
    Bool,
    Void
};

struct TypeSyntax {
    TypeKind kind;
    SourceSpan span;
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
    GreaterEqual,

    LogicalAnd,
    LogicalOr
};

struct Expr {
    SourceSpan span;
    virtual ~Expr() = default;
};

struct IntegerLiteralExpr : Expr {
    std::int64_t value;
};

struct BoolLiteralExpr : Expr {
    bool value;
};

struct VariableExpr : Expr {
    std::string name;
};

struct UnaryExpr : Expr {
    UnaryOp op;
    std::unique_ptr<Expr> operand;
};

struct BinaryExpr : Expr {
    BinaryOp op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
};

struct CallExpr : Expr {
    std::string callee;
    std::vector<std::unique_ptr<Expr>> arguments;
};

}