#include "compiler/semantic/semantic_analyzer.hpp"

#include <optional>
#include <utility>
#include <cassert>
#include <cstddef>

namespace compiler {

SemanticResult SemanticAnalyzer::collect_functions(const Program& program) {
    for (const FunctionDecl& fn : program.functions) {
        if (functions_.contains(fn.name)) {
            return SemanticError{
                fn.name_span,
                "duplicate function declaration"
            };
        }

        FunctionSymbol symbol;
        

        for (const Parameter& parameter : fn.parameters) {
            if (parameter.type.kind == TypeKind::Void) {
                return SemanticError{
                    parameter.type.span,
                    "parameter cannot have type void"
                };
            }
            symbol.parameter_types.push_back(parameter.type.kind);
        }

        symbol.return_type = fn.return_type.kind;
        symbol.declaration_span = fn.name_span;

        functions_.emplace(fn.name, std::move(symbol));
    }

    return std::nullopt;
}

SemanticResult SemanticAnalyzer::analyze_function(const FunctionDecl& function) {
    current_return_type_ = function.return_type.kind;

    push_scope();

    for (const Parameter& parameter : function.parameters) {
        SemanticResult decl_result = declare_variable(
            parameter.name,
            parameter.type.kind,
            parameter.name_span
        );

        if (decl_result.has_value()) {
            pop_scope();
            return decl_result;
        }
    }

    SemanticResult analyze_result = analyze_statements(*function.body);

    pop_scope();

    return analyze_result;
}

SemanticResult SemanticAnalyzer::analyze_block(const BlockStmt& block){
    push_scope();

    SemanticResult analyze_result = analyze_statements(block);

    pop_scope();

    return analyze_result;
}

SemanticResult SemanticAnalyzer::analyze_statements(const BlockStmt& block) {
    for (const StmtPtr& statement : block.statements) {
        SemanticResult analyze_result = analyze_statement(*statement);

        if (analyze_result.has_value()) {
            return analyze_result;
        }
    }

    return std::nullopt;
}


SemanticResult SemanticAnalyzer::analyze_statement(const Stmt& statement) {
    // Variable declarations
    if (const auto* decl = dynamic_cast<const VariableDeclStmt*>(&statement)) {

        if (decl->type.kind == TypeKind::Void) {
            return SemanticError{
                decl->type.span,
                "variable cannot have type void"
            };
        }

        if (decl->initializer) {
            ExprTypeResult initializer = analyze_expression(*decl->initializer);

            if (std::holds_alternative<SemanticError>(initializer)) {
                return std::get<SemanticError>(initializer);
            }

            if (std::get<TypeKind>(initializer) != decl->type.kind) {
                return SemanticError{
                    decl->initializer->span,
                    "initializer type does not match variable type"
                };
            }
        }

        return declare_variable(
            decl->name,
            decl->type.kind,
            decl->name_span
        );
    }

    // Assignments
    if (const auto* assignment = dynamic_cast<const AssignmentStmt*>(&statement)) {

        const VariableSymbol* variable = lookup_variable(assignment->name);

        if (variable == nullptr) {
            return SemanticError{
                assignment->name_span,
                "assignment to undeclared variable: " + assignment->name
            };
        }

        ExprTypeResult value = analyze_expression(*assignment->value);

        if (std::holds_alternative<SemanticError>(value)) {
            return std::get<SemanticError>(value);
        }

        if (std::get<TypeKind>(value) != variable->type) {
            return SemanticError{
                assignment->value->span,
                "assignment type does not match variable type"
            };
        }

        return std::nullopt;
    }

    // If statements
    if (const auto* if_stmt = dynamic_cast<const IfStmt*>(&statement)) {

        ExprTypeResult condition = analyze_expression(*if_stmt->condition);

        if (std::holds_alternative<SemanticError>(condition)) {
            return std::get<SemanticError>(condition);
        }

        if (std::get<TypeKind>(condition) != TypeKind::Bool) {
            return SemanticError{
                if_stmt->condition->span,
                "if condition must be bool"
            };
        }

        SemanticResult result = analyze_block(*if_stmt->then_branch);

        if (result.has_value()) {
            return result;
        }

        if (if_stmt->else_branch) {
            result = analyze_block(*if_stmt->else_branch);

            if (result.has_value()) {
                return result;
            }
        }

        return std::nullopt;
    }

    // While statements
    if (const auto* while_stmt = dynamic_cast<const WhileStmt*>(&statement)) {

        ExprTypeResult condition = analyze_expression(*while_stmt->condition);

        if (std::holds_alternative<SemanticError>(condition)) {
            return std::get<SemanticError>(condition);
        }

        if (std::get<TypeKind>(condition) != TypeKind::Bool) {
            return SemanticError{
                while_stmt->condition->span,
                "while condition must be bool"
            };
        }

        return analyze_block(*while_stmt->body);
    }

    // Return statements
    if (const auto* return_stmt = dynamic_cast<const ReturnStmt*>(&statement)) {

        if (return_stmt->value == nullptr) {
            if (current_return_type_ != TypeKind::Void) {
                return SemanticError{
                    return_stmt->span,
                    "non-void function must return a value"
                };
            }

            return std::nullopt;
        }

        if (current_return_type_ == TypeKind::Void) {
            return SemanticError{
                return_stmt->span,
                "void function cannot return a value"
            };
        }

        ExprTypeResult value = analyze_expression(*return_stmt->value);

        if (std::holds_alternative<SemanticError>(value)) {
            return std::get<SemanticError>(value);
        }

        if (std::get<TypeKind>(value) != current_return_type_) {
            return SemanticError{
                return_stmt->value->span,
                "return type does not match function return type"
            };
        }

        return std::nullopt;
    }

    // Expression statements
    if (const auto* expr_stmt =dynamic_cast<const ExprStmt*>(&statement)) {

        ExprTypeResult result = analyze_expression(*expr_stmt->expression);

        if (std::holds_alternative<SemanticError>(result)) {
            return std::get<SemanticError>(result);
        }

        return std::nullopt;
    }

    return SemanticError{
        statement.span,
        "unsupported statement type"
    };
}


ExprTypeResult SemanticAnalyzer::analyze_expression(const Expr& expression){
    if (dynamic_cast<const IntegerLiteralExpr*>(&expression)) {
        return TypeKind::I64;
    }

    if (dynamic_cast<const BoolLiteralExpr*>(&expression)) {
        return TypeKind::Bool;
    }

    if (const auto* var = dynamic_cast<const VariableExpr*>(&expression)) {
        const VariableSymbol* symbol = lookup_variable(var->name);

        if (symbol == nullptr){
            return SemanticError{
                var->span,
                "undeclared variable: " + var->name
            };
        }

        return symbol->type;
    }

    if (const auto* unary = dynamic_cast<const UnaryExpr*>(&expression)) {

        ExprTypeResult operand_result = analyze_expression(*unary->operand);

        if (std::holds_alternative<SemanticError>(operand_result)) {
            return std::get<SemanticError>(operand_result);
        }
        
        TypeKind operand_type = std::get<TypeKind>(operand_result);
        
        if(unary->op == UnaryOp::Negate) {
            if(operand_type != TypeKind::I64){
                return SemanticError{
                    unary->span,
                    "unary negation requires i64"
                };
            }
            
            return TypeKind::I64;
        }

        if(unary->op == UnaryOp::LogicalNot) {
            if(operand_type != TypeKind::Bool){
                return SemanticError{
                    unary->span,
                    "Logical Not requires Bool"
                };
            }

            return TypeKind::Bool;
        }

        return SemanticError{
            unary->span,
            "unsupported unary operator"
        };
    }

    if (const auto* binary = dynamic_cast<const BinaryExpr*>(&expression)) {

        ExprTypeResult left_result = analyze_expression(*binary->left);

        if (std::holds_alternative<SemanticError>(left_result)) {
            return std::get<SemanticError>(left_result);
        }

        ExprTypeResult right_result =
            analyze_expression(*binary->right);

        if (std::holds_alternative<SemanticError>(right_result)) {
            return std::get<SemanticError>(right_result);
        }

        TypeKind left_type = std::get<TypeKind>(left_result);
        TypeKind right_type = std::get<TypeKind>(right_result);

        if (binary->op == BinaryOp::Add ||
            binary->op == BinaryOp::Subtract ||
            binary->op == BinaryOp::Multiply ||
            binary->op == BinaryOp::Divide ||
            binary->op == BinaryOp::Remainder) {

            if (left_type != TypeKind::I64 ||
                right_type != TypeKind::I64) {
                return SemanticError{
                    binary->span,
                    "arithmetic operands must be i64"
                };
            }

            return TypeKind::I64;
        }

        if (binary->op == BinaryOp::Less ||
            binary->op == BinaryOp::LessEqual ||
            binary->op == BinaryOp::Greater ||
            binary->op == BinaryOp::GreaterEqual) {

            if (left_type != TypeKind::I64 ||
                right_type != TypeKind::I64) {
                return SemanticError{
                    binary->span,
                    "relational operands must be i64"
                };
            }

            return TypeKind::Bool;
        }

        if (binary->op == BinaryOp::Equal ||
            binary->op == BinaryOp::NotEqual) {

            if (left_type != right_type ||
                (left_type != TypeKind::I64 &&
                left_type != TypeKind::Bool)) {
                return SemanticError{
                    binary->span,
                    "equality operands must have matching i64 or bool types"
                };
            }

            return TypeKind::Bool;
        }

        if (binary->op == BinaryOp::LogicalAnd ||
            binary->op == BinaryOp::LogicalOr) {

            if (left_type != TypeKind::Bool ||
                right_type != TypeKind::Bool) {
                return SemanticError{
                    binary->span,
                    "logical operands must be bool"
                };
            }

            return TypeKind::Bool;
        }

        return SemanticError{
            binary->span,
            "unsupported binary operator"
        };
    }
    
    if (const auto* call = dynamic_cast<const CallExpr*>(&expression)) {
        auto it = functions_.find(call->callee);

        if (it == functions_.end()) {
            return SemanticError{
                call->span,
                "undeclared function: " + call->callee
            };
        }

        const FunctionSymbol& function = it->second;

        if (call->arguments.size() != function.parameter_types.size()) {
            return SemanticError{
                call->span,
                "wrong number of arguments in call to " + call->callee
            };
        }

        for (std::size_t i = 0; i < call->arguments.size(); ++i) {
            const Expr& argument = *call->arguments[i];

            ExprTypeResult result = analyze_expression(argument);

            if (std::holds_alternative<SemanticError>(result)) {
                return std::get<SemanticError>(result);
            }

            if (std::get<TypeKind>(result) != function.parameter_types[i]) {
                return SemanticError{
                    argument.span,
                    "argument type mismatch in call to " + call->callee
                };
            }
        }

        return function.return_type;
    }

    return SemanticError{expression.span, "unsupported expression type"};
}

SemanticResult SemanticAnalyzer::analyze(const Program& program) {
    functions_.clear();
    scopes_.clear();

    SemanticResult collect_result = collect_functions(program);

    if (collect_result.has_value()) {
        return collect_result;
    }

    for (const FunctionDecl& fn : program.functions) {
        SemanticResult analyze_result = analyze_function(fn);

        if (analyze_result.has_value()) {
            return analyze_result;
        }
    }

    return std::nullopt;
}

void SemanticAnalyzer::push_scope() {
    scopes_.emplace_back();
}

void SemanticAnalyzer::pop_scope() {
    assert(!scopes_.empty());
    scopes_.pop_back();
}

SemanticResult SemanticAnalyzer::declare_variable(const std::string& name, TypeKind type, SourceSpan declaration_span) {
    assert(!scopes_.empty());
    if (scopes_.back().contains(name)) {
        return SemanticError{declaration_span, "duplicate variable declaration"};
    }
    scopes_.back().emplace(name, VariableSymbol{type, declaration_span});

    return std::nullopt;
}

const VariableSymbol* SemanticAnalyzer::lookup_variable(const std::string& name) const{
    assert(!scopes_.empty());
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it){
        auto found = it->find(name);

        if(found != it->end()){
            return &found->second;
        }
    }

    return nullptr;
}

} // namespace compiler