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

using ExprPtr = std::unique_ptr<Expr>;

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
    ExprPtr operand;
};

struct BinaryExpr : Expr {
    BinaryOp op;
    ExprPtr left;
    ExprPtr right;
};

struct CallExpr : Expr {
    std::string callee;
    std::vector<ExprPtr> arguments;
};


struct Stmt {
    SourceSpan span;
    virtual ~Stmt() = default;
};

using StmtPtr = std::unique_ptr<Stmt>;

struct BlockStmt : Stmt {
    std::vector<StmtPtr> statements;
};

struct VariableDeclStmt : Stmt {
    std::string name;
    SourceSpan name_span;
    TypeSyntax type;
    ExprPtr initializer;      // nullptr means no initializer
};

struct AssignmentStmt : Stmt {
    std::string name;
    SourceSpan name_span;
    ExprPtr value;
};

struct IfStmt : Stmt {
    ExprPtr condition;
    std::unique_ptr<BlockStmt> then_branch;
    std::unique_ptr<BlockStmt> else_branch; // nullptr if absent
};

struct WhileStmt : Stmt {
    ExprPtr condition;
    std::unique_ptr<BlockStmt> body;
};

struct ReturnStmt : Stmt {
    ExprPtr value;            // nullptr for `return;`
};

struct ExprStmt : Stmt {
    ExprPtr expression;
};


struct Parameter {
    std::string name;
    SourceSpan name_span;
    TypeSyntax type;
    SourceSpan span;
};

struct FunctionDecl {
    std::string name;
    SourceSpan name_span;
    std::vector<Parameter> parameters;
    TypeSyntax return_type;
    std::unique_ptr<BlockStmt> body;
    SourceSpan span;
};

struct Program {
    std::vector<FunctionDecl> functions;
};

}