# x86-64 Native Compiler

A standalone C++20 compiler for a small statically typed language, built from first principles with the goal of generating native x86-64 code.

The compiler uses a handwritten frontend and is being developed through the full compilation pipeline: parsing, semantic analysis, compiler-owned intermediate representation, optimization, machine lowering, register allocation, and x86-64 code generation.

The current implementation includes **source infrastructure, a handwritten lexer, recursive-descent parser, abstract syntax tree (AST), and semantic analyzer** with lexical scope resolution, function signature validation, and static type checking.

The next major stage is a **non-SSA intermediate representation (IR)** with explicit basic blocks, control flow, and mutable local storage.

## Compiler Pipeline

```text
Source
  |
  v
Lexer
  |
  v
Token Stream
  |
  v
Parser
  |
  v
AST
  |
  v
Semantic Analysis
  |
  v
Non-SSA IR              <-- Next
  |
  v
IR Verification
  |
  v
CFG / Dataflow Analysis
  |
  v
Optimization / SSA
  |
  v
Machine Lowering
  |
  v
Instruction Selection
  |
  v
Register Allocation
  |
  v
x86-64 Code Generation
```

The diagram represents the major architectural stages rather than a strictly linear implementation. Verification will be used throughout IR development, while machine-level analysis and transformations may require iteration.

The project intentionally keeps the major compiler stages explicit rather than delegating the pipeline to an existing compiler framework.

## Technical Highlights

### Frontend

- Handwritten lexer and recursive-descent parser
- Byte-offset source spans retained through lexical, syntactic, and semantic analysis
- Offset-to-line/column mapping for source diagnostics
- Explicit operator precedence and associativity in expression parsing
- Structured lexical, parse, and semantic errors
- AST ownership expressed using C++ smart pointers
- `std::variant`- and `std::optional`-based result/error representation
- Semantic analysis with lexical scope resolution, function symbol collection, and static type checking
- Function-call validation supporting forward references and recursion
- C++20 implementation with strict compiler warnings enabled during development
- Automated testing with GoogleTest and CTest

### Parser

The parser constructs the AST directly from the token stream and handles expression precedence across:

```text
logical OR
    |
logical AND
    |
equality
    |
comparison
    |
addition / subtraction
    |
multiplication / division / remainder
    |
unary expressions
    |
primary expressions
```

The frontend supports syntax for functions, local variables, control flow, expressions, function calls, and returns.

### Source and Diagnostics

Source locations are represented using byte-offset spans into the original source file.

This allows tokens, parse errors, and semantic errors to retain their connection to the input source while keeping the internal representation compact. Source offsets can be translated to line and column locations when diagnostics are presented to the user.

### Semantic Analysis

The semantic analyzer traverses the AST and validates symbol resolution, lexical scoping, expression types, function signatures, and statement-level type constraints.

Key capabilities:

- **Function resolution:** Collects function signatures before analyzing bodies, supporting forward references and recursive calls. Detects duplicate function declarations.
- **Lexical scoping:** Maintains nested variable symbol tables, supports shadowing, and rejects duplicate declarations within the same scope.
- **Expression type checking:** Validates arithmetic, relational, equality, logical, and unary operators for the `i64` and `bool` types.
- **Function-call validation:** Checks function existence, argument counts, and argument types against declared signatures.
- **Statement validation:** Checks variable initializers, assignments, boolean conditions in `if`/`while` statements, and return-value compatibility.
- **Source-aware diagnostics:** Reports semantic errors with source spans identifying offending declarations or expressions.

Function signatures are collected before individual function bodies are analyzed, allowing functions to reference definitions appearing later in the source.

Variable resolution uses nested lexical scopes, with declarations resolved from the innermost scope outward.

Semantic analysis is covered by GoogleTest unit tests for valid programs, invalid type combinations, scope resolution, function calls, and error reporting.

## Language

The language is intentionally small so that the complete compiler pipeline can remain understandable and independently implementable.

### Types

```text
i64
bool
void
```

### Core Constructs

- Functions
- Parameters and local variables
- Arithmetic expressions
- Comparison expressions
- Boolean expressions
- Function calls
- `if` / `else`
- `while`
- `return`
- Recursion

A representative program looks like:

```text
fn factorial(n: i64) -> i64 {
    if (n <= 1) {
        return 1;
    }

    return n * factorial(n - 1);
}
```

The language is deliberately constrained: the focus of the project is compiler architecture, intermediate representations, optimization, and native code generation rather than designing a large programming language.

## Current Status

### Implemented

**Source Infrastructure**
- Source-file loading and error handling
- Source offsets and spans
- Line/column mapping
- Command-line compiler driver

**Lexical Analysis**
- Identifiers and reserved keywords
- Integer literals
- Arithmetic operators
- Comparison operators
- Logical operators
- Delimiters and punctuation
- Whitespace and `//` comments
- Source-aware lexical errors

**Abstract Syntax Tree**
- Primitive type representation
- Literal expressions
- Variable expressions
- Unary and binary expressions
- Function calls
- Statements and declarations
- Functions and control-flow constructs
- Explicit tree ownership

**Parsing**
- Recursive-descent parsing
- Precedence-aware expression parsing
- Function and statement parsing
- AST construction
- Source-aware parse errors

**Semantic Analysis**
- Function symbol collection and duplicate detection
- Lexical scope creation and variable resolution
- Duplicate variable and parameter detection
- Static expression type checking
- Function-call signature validation
- Assignment and initializer checking
- Conditional expression validation
- Return type checking
- Source-aware semantic errors

### Next Stage: Non-SSA Intermediate Representation

The next stage introduces a compiler-owned intermediate representation designed to support control-flow analysis, optimization, and eventual lowering to x86-64.

The initial IR will distinguish between:

- **Values:** Typed temporary results produced by computations
- **Locals:** Mutable storage locations that can be explicitly loaded and stored
- **Basic blocks:** Sequences of instructions terminated by explicit control-flow operations
- **Functions:** Collections of basic blocks, local storage, parameters, and function metadata

The initial milestone focuses on defining the IR data structures and producing a deterministic textual representation.

AST-to-IR lowering, verification, control-flow analysis, and optimization will follow as separate implementation stages.

## Development Roadmap

The project is being developed incrementally so that each stage can be tested and reasoned about independently.

- [x] Source infrastructure
- [x] Lexer
- [x] Recursive-descent parser and AST
- [x] Semantic analysis and type checking
- [ ] Non-SSA IR representation and textual dumper
- [ ] AST-to-IR lowering
- [ ] IR verifier
- [ ] Control-flow graph and dataflow analysis
- [ ] Initial optimization passes
- [ ] SSA construction where useful
- [ ] Machine-oriented lowering
- [ ] x86-64 instruction selection
- [ ] Liveness analysis and register allocation
- [ ] System V AMD64 ABI and stack-frame construction
- [ ] Native assembly generation
- [ ] Generated-code analysis and performance evaluation

### Backend Direction

The planned backend will implement x86-64 instruction selection, liveness analysis, register allocation and spilling, stack-frame construction, and System V AMD64 calling-convention support.

Later work will investigate generated-code quality, targeted machine-level optimizations, and comparisons against appropriate GCC/Clang baselines.

LLVM and MLIR experimentation is intentionally deferred until the standalone compiler has established a first-principles foundation for comparing compiler architectures, intermediate representations, and optimization strategies.

## Building

### Requirements

- C++20 compiler
- CMake 3.20+
- Ninja

Configure and build:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

## Testing

The project uses GoogleTest with test discovery through CTest.

Run the test suite with:

```bash
ctest --test-dir build --output-on-failure
```

Tests are organized around individual compiler components so that frontend behavior can be validated independently as the pipeline grows.

Current test coverage includes:

- **Source infrastructure:** Source loading, byte offsets, line/column mapping, and boundary conditions
- **Lexer:** Token recognition, keywords, operators, whitespace, comments, source spans, and lexical errors
- **Parser:** AST construction, operator precedence, declarations, statements, control flow, and syntax errors
- **Semantic analyzer:** Function declarations, lexical scopes, shadowing, type checking, function calls, assignments, control-flow conditions, and return statements

As the compiler develops, tests will expand to cover IR invariants, lowering correctness, optimization behavior, and generated native code.

## Current Limitations

- The compiler currently implements frontend analysis but does not yet generate intermediate representation or native machine code.
- Semantic analysis validates symbol resolution, types, function signatures, and return-value compatibility, but does not yet enforce definite assignment or guaranteed returns along every control-flow path.
- The language supports only `i64`, `bool`, and `void`, with a deliberately restricted set of statements and expressions.
- Function overloading is not supported.
- Optimization, register allocation, calling-convention implementation, and native code generation remain planned work.

These limitations reflect the project's incremental development strategy rather than intended final compiler capabilities.

## Project Goals

This project is intended to exercise the parts of compiler engineering that become important beyond basic parsing:

- Semantic analysis and language invariants
- Intermediate representation design
- Control-flow and dataflow analysis
- Optimization
- SSA construction and transformation
- Machine-level lowering
- Instruction selection
- Register allocation
- ABI implementation
- Native code generation
- Generated-code quality analysis

The long-term goal is a compiler where the path from source program to native x86-64 code remains explicit enough to inspect, test, measure, and reason about at every stage.