#pragma once

#include "compiler/ast/ast.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <variant>

namespace compiler {

struct SemanticError {
    SourceSpan span;
    std::string message;
};

using SemanticResult = std::optional<SemanticError>;
using ExprTypeResult = std::variant<TypeKind, SemanticError>;


struct FunctionSymbol {
    std::vector<TypeKind> parameter_types;
    TypeKind return_type;
    SourceSpan declaration_span;
};

struct VariableSymbol {
    TypeKind type;
    SourceSpan declaration_span;
};

using FunctionTable = std::unordered_map<std::string, FunctionSymbol>;
using Scope = std::unordered_map<std::string, VariableSymbol>;

class SemanticAnalyzer {
public:
    [[nodiscard]]
    SemanticResult analyze(const Program& program);

private:
    FunctionTable functions_;

    TypeKind current_return_type_ = TypeKind::Void;

    SemanticResult collect_functions(const Program& program);
    SemanticResult analyze_function(const FunctionDecl& function);
    SemanticResult analyze_block(const BlockStmt& block);
    SemanticResult analyze_statements(const BlockStmt& block);
    SemanticResult analyze_statement(const Stmt& statement);
    
    ExprTypeResult analyze_expression(const Expr& expression);
    
    std::vector<Scope> scopes_;

    void push_scope();
    void pop_scope();

    SemanticResult declare_variable(
        const std::string& name,
        TypeKind type,
        SourceSpan declaration_span
    );

    const VariableSymbol* lookup_variable(
        const std::string& name
    ) const;
    
};

}