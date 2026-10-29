#include "compiler/ast/ast.hpp"
#include "compiler/lexer/lexer.hpp"
#include "compiler/parser/parser.hpp"
#include "compiler/semantic/semantic_analyzer.hpp"
#include "compiler/source/source_file.hpp"

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace {

compiler::SemanticResult analyze_test_program(compiler::SemanticAnalyzer& analyzer, std::string_view text) {
    compiler::SourceFile source{"test.xc", std::string{text}};
    compiler::Lexer lexer{source};
    std::vector<compiler::Token> tokens;

    while (true) {
        compiler::LexResult result = lexer.next();
        if (std::holds_alternative<compiler::LexError>(result)) {
            ADD_FAILURE() << "test input unexpectedly failed lexing";
            return compiler::SemanticError{{0, 0}, "test setup failed to lex"};
        }

        compiler::Token token = std::get<compiler::Token>(result);
        tokens.push_back(token);
        if (token.kind == compiler::TokenKind::Eof) {
            break;
        }
    }

    compiler::Parser parser{source, std::move(tokens)};
    compiler::ProgramParseResult parsed = parser.parse_program();

    if (std::holds_alternative<compiler::ParseError>(parsed)) {
        const auto& error = std::get<compiler::ParseError>(parsed);
        ADD_FAILURE()
            << "test input unexpectedly failed parsing: "
            << error.message;
        return compiler::SemanticError{error.span, "test setup failed to parse"};
    }

    return analyzer.analyze(std::get<compiler::Program>(parsed));
}

compiler::SemanticResult analyze_test_program(std::string_view text) {
    compiler::SemanticAnalyzer analyzer;
    return analyze_test_program(analyzer, text);
}

void expect_valid(std::string_view source) {
    compiler::SemanticResult result = analyze_test_program(source);
    EXPECT_FALSE(result.has_value()) << (result.has_value() ? result->message : "");
}

void expect_error(std::string_view source, std::string_view message) {
    compiler::SemanticResult result = analyze_test_program(source);
    ASSERT_TRUE(result.has_value()) << "expected a semantic error";
    EXPECT_EQ(result->message, message);
}

} // namespace

//Function declarations

TEST(SemanticTest, UniqueFunctions) {
    expect_valid(R"(
fn add(a: i64, b: i64) -> i64 {
    return a + b;
}
fn main() -> i64 {
    return add(1, 2);
}
)");
}

TEST(SemanticTest, DuplicateFunctions) {
    const std::string_view input = R"(
fn foo() -> void {
}

fn foo(x: i64) -> i64 {
    return x;
}
)";
    compiler::SemanticResult result = analyze_test_program(input);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->message, "duplicate function declaration");
    EXPECT_EQ(result->span.begin, 26u);
    EXPECT_EQ(result->span.end, 29u);
}

TEST(SemanticTest, AnalyzerReuse) {
    compiler::SemanticAnalyzer analyzer;

    auto first = analyze_test_program(analyzer, R"(
fn foo() -> void {}
)");
    ASSERT_FALSE(first.has_value());

    auto second = analyze_test_program(analyzer, R"(
fn foo() -> i64 { return 42; }
)");
    EXPECT_FALSE(second.has_value());

    auto third = analyze_test_program(analyzer, R"(
fn bar() -> void {}
fn bar() -> void {}
)");
    ASSERT_TRUE(third.has_value());
    EXPECT_EQ(third->message, "duplicate function declaration");

    auto fourth = analyze_test_program(analyzer, R"(
fn bar() -> void {}
)");
    EXPECT_FALSE(fourth.has_value());
}

TEST(SemanticTest, RejectsVoidParameter) {
    expect_error(R"(
fn foo(x: void) -> void {}
)", "parameter cannot have type void");
}

TEST(SemanticTest, AllowsVoidReturnType) {
    expect_valid(R"(
fn foo() -> void { return; }
)");
}

//Variables and lexical scopes

TEST(SemanticTest, ResolvesParameter) {
    expect_valid(R"(
fn identity(x: i64) -> i64 { return x; }
)");
}

TEST(SemanticTest, ResolvesLocalVariable) {
    expect_valid(R"(
fn main() -> i64 {
    let x: i64 = 7;
    return x;
}
)");
}

TEST(SemanticTest, ResolvesOuterVariableFromNestedScope) {
    expect_valid(R"(
fn main() -> i64 {
    let x: i64 = 7;
    if (true) {
        x = x + 1;
    }
    return x;
}
)");
}

TEST(SemanticTest, RejectsUndefinedVariable) {
    expect_error(R"(
fn main() -> i64 { return missing; }
)", "undeclared variable: missing");
}

TEST(SemanticTest, RejectsSameScopeDuplicateVariable) {
    expect_error(R"(
fn main() -> void {
    let x: i64 = 1;
    let x: bool = true;
}
)", "duplicate variable declaration");
}

TEST(SemanticTest, RejectsDuplicateParameters) {
    expect_error(R"(
fn foo(x: i64, x: bool) -> i64 { return 0; }
)", "duplicate variable declaration");
}

TEST(SemanticTest, RejectsParameterRedeclarationInFunctionBody) {
    expect_error(R"(
fn foo(x: i64) -> i64 {
    let x: i64 = 2;
    return x;
}
)", "duplicate variable declaration");
}

TEST(SemanticTest, AllowsNestedShadowing) {
    expect_valid(R"(
fn main() -> i64 {
    let x: i64 = 4;
    if (true) {
        let x: bool = true;
        x = false;
    }
    return x;
}
)");
}

TEST(SemanticTest, AllowsSameNameInSeparateBranches) {
    expect_valid(R"(
fn main() -> void {
    if (true) {
        let x: i64 = 1;
    } else {
        let x: bool = false;
    }
}
)");
}

TEST(SemanticTest, RejectsUseAfterLeavingIfScope) {
    expect_error(R"(
fn main() -> i64 {
    if (true) { let hidden: i64 = 1; }
    return hidden;
}
)", "undeclared variable: hidden");
}

TEST(SemanticTest, RejectsUseAfterLeavingWhileScope) {
    expect_error(R"(
fn main() -> i64 {
    while (false) { let hidden: i64 = 1; }
    return hidden;
}
)", "undeclared variable: hidden");
}

TEST(SemanticTest, RejectsVariableUsedBeforeDeclaration) {
    expect_error(R"(
fn main() -> i64 {
    let y: i64 = x;
    let x: i64 = 1;
    return y;
}
)", "undeclared variable: x");
}

TEST(SemanticTest, RejectsSelfReferentialInitializer) {
    expect_error(R"(
fn main() -> i64 {
    let x: i64 = x + 1;
    return x;
}
)", "undeclared variable: x");
}

TEST(SemanticTest, ShadowingInitializerCanReadOuterVariable) {
    expect_valid(R"(
fn main() -> i64 {
    let x: i64 = 4;
    if (true) {
        let x: i64 = x + 1;
    }
    return x;
}
)");
}

TEST(SemanticTest, RejectsVoidVariable) {
    expect_error(R"(
fn main() -> void { let x: void; }
)", "variable cannot have type void");
}

//Unary and binary expressions

TEST(SemanticTest, AcceptsUnaryOperators) {
    expect_valid(R"(
fn negative() -> i64 { return -42; }
fn inverted() -> bool { return !false; }
)");
}

TEST(SemanticTest, RejectsNegationOfBool) {
    expect_error(R"(
fn main() -> i64 { return -true; }
)", "unary negation requires i64");
}

TEST(SemanticTest, RejectsLogicalNotOfI64) {
    expect_error(R"(
fn main() -> bool { return !42; }
)", "Logical Not requires Bool");
}

TEST(SemanticTest, AcceptsArithmeticOperators) {
    expect_valid(R"(
fn math(a: i64, b: i64) -> i64 {
    return (a + b) * (a - b) / 2 % 3;
}
)");
}

TEST(SemanticTest, RejectsArithmeticWithBool) {
    expect_error(R"(
fn main() -> i64 { return 1 + true; }
)", "arithmetic operands must be i64");
}

TEST(SemanticTest, AcceptsRelationalOperators) {
    expect_valid(R"(
fn compare(a: i64, b: i64) -> bool {
    return a < b || a <= b || a > b || a >= b;
}
)");
}

TEST(SemanticTest, RejectsRelationalComparisonOfBools) {
    expect_error(R"(
fn main() -> bool { return true < false; }
)", "relational operands must be i64");
}

TEST(SemanticTest, AcceptsEqualityForI64AndBool) {
    expect_valid(R"(
fn main() -> bool {
    return (1 == 1) && (true != false) && (2 != 3);
}
)");
}

TEST(SemanticTest, RejectsEqualityBetweenDifferentTypes) {
    expect_error(R"(
fn main() -> bool { return 1 == true; }
)", "equality operands must have matching i64 or bool types");
}

TEST(SemanticTest, AcceptsLogicalOperators) {
    expect_valid(R"(
fn main() -> bool { return (true && false) || !false; }
)");
}

TEST(SemanticTest, RejectsLogicalOperatorsOnIntegers) {
    expect_error(R"(
fn main() -> bool { return 1 && 2; }
)", "logical operands must be bool");
}

TEST(SemanticTest, ChecksNestedExpressionTypes) {
    expect_valid(R"(
fn main() -> bool {
    let a: i64 = 10;
    return -(a + 5) < 0 && (a % 2 == 0);
}
)");
}

//Function calls

TEST(SemanticTest, AcceptsFunctionCall) {
    expect_valid(R"(
fn add(a: i64, b: i64) -> i64 { return a + b; }
fn main() -> i64 { return add(10, 20); }
)");
}

TEST(SemanticTest, AcceptsForwardFunctionCall) {
    expect_valid(R"(
fn main() -> bool { return later(4); }
fn later(x: i64) -> bool { return x > 0; }
)");
}

TEST(SemanticTest, AcceptsRecursiveFunctionCall) {
    expect_valid(R"(
fn countdown(n: i64) -> i64 {
    if (n == 0) { return 0; }
    return countdown(n - 1);
}
)");
}

TEST(SemanticTest, RejectsUndefinedFunction) {
    expect_error(R"(
fn main() -> i64 { return missing(1); }
)", "undeclared function: missing");
}

TEST(SemanticTest, RejectsWrongArgumentCount) {
    expect_error(R"(
fn add(a: i64, b: i64) -> i64 { return a + b; }
fn main() -> i64 { return add(1); }
)", "wrong number of arguments in call to add");
}

TEST(SemanticTest, RejectsWrongArgumentType) {
    expect_error(R"(
fn add(a: i64, b: i64) -> i64 { return a + b; }
fn main() -> i64 { return add(true, 2); }
)", "argument type mismatch in call to add");
}

TEST(SemanticTest, RejectsInvalidNestedCallArgument) {
    expect_error(R"(
fn identity(x: i64) -> i64 { return x; }
fn main() -> i64 { return identity(unknown); }
)", "undeclared variable: unknown");
}

TEST(SemanticTest, AcceptsVoidFunctionCallAsStatement) {
    expect_valid(R"(
fn noop() -> void { return; }
fn main() -> void { noop(); }
)");
}

TEST(SemanticTest, RejectsVoidFunctionCallAsI64Value) {
    expect_error(R"(
fn noop() -> void { return; }
fn main() -> i64 { return noop(); }
)", "return type does not match function return type");
}

TEST(SemanticTest, ChecksFunctionReturnTypeInParentExpression) {
    expect_error(R"(
fn is_ready() -> bool { return true; }
fn main() -> i64 { return is_ready() + 1; }
)", "arithmetic operands must be i64");
}

//Statements, conditions, returns

TEST(SemanticTest, AcceptsMatchingInitializer) {
    expect_valid(R"(
fn main() -> bool {
    let ready: bool = true;
    return ready;
}
)");
}

TEST(SemanticTest, RejectsInitializerTypeMismatch) {
    expect_error(R"(
fn main() -> void { let count: i64 = false; }
)", "initializer type does not match variable type");
}

TEST(SemanticTest, AcceptsAssignmentWithMatchingType) {
    expect_valid(R"(
fn main() -> void {
    let count: i64 = 0;
    count = count + 1;
}
)");
}

TEST(SemanticTest, RejectsAssignmentToUndefinedVariable) {
    expect_error(R"(
fn main() -> void { missing = 1; }
)", "assignment to undeclared variable: missing");
}

TEST(SemanticTest, RejectsAssignmentTypeMismatch) {
    expect_error(R"(
fn main() -> void {
    let count: i64 = 1;
    count = true;
}
)", "assignment type does not match variable type");
}

TEST(SemanticTest, AcceptsBoolIfAndWhileConditions) {
    expect_valid(R"(
fn main() -> void {
    let count: i64 = 0;
    while (count < 3) {
        if (count == 1) { count = count + 1; }
        else { count = count + 2; }
    }
}
)");
}

TEST(SemanticTest, RejectsNonBoolIfCondition) {
    expect_error(R"(
fn main() -> void { if (123) {} }
)", "if condition must be bool");
}

TEST(SemanticTest, RejectsNonBoolWhileCondition) {
    expect_error(R"(
fn main() -> void { while (123) {} }
)", "while condition must be bool");
}

TEST(SemanticTest, RejectsUndeclaredConditionVariable) {
    expect_error(R"(
fn main() -> void { if (missing) {} }
)", "undeclared variable: missing");
}

TEST(SemanticTest, AcceptsNonVoidReturnOfCorrectType) {
    expect_valid(R"(
fn main() -> i64 { return 42; }
)");
}

TEST(SemanticTest, AcceptsBareReturnInVoidFunction) {
    expect_valid(R"(
fn main() -> void { return; }
)");
}

TEST(SemanticTest, RejectsMissingReturnValueInNonVoidFunction) {
    expect_error(R"(
fn main() -> i64 { return; }
)", "non-void function must return a value");
}

TEST(SemanticTest, RejectsValueReturnedFromVoidFunction) {
    expect_error(R"(
fn main() -> void { return 42; }
)", "void function cannot return a value");
}

TEST(SemanticTest, RejectsReturnTypeMismatch) {
    expect_error(R"(
fn main() -> bool { return 42; }
)", "return type does not match function return type");
}

TEST(SemanticTest, ChecksExpressionStatements) {
    expect_valid(R"(
fn main() -> void {
    1 + 2;
    true != false;
}
)");
}

TEST(SemanticTest, RejectsInvalidExpressionStatement) {
    expect_error(R"(
fn main() -> void { missing(1); }
)", "undeclared function: missing");
}
