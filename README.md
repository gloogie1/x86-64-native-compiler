# x86-64 Native Compiler

A standalone C++20 compiler for a small statically typed language targeting native x86-64 code.

The project is being built from first principles to explore compiler construction beyond frontend parsing, with a planned pipeline covering semantic analysis, intermediate representations, control-flow and dataflow analysis, optimization, machine lowering, register allocation, and native x86-64 code generation.

## Current Status

Frontend infrastructure and lexical analysis are currently implemented.

### Implemented

* Source-file abstraction with byte-offset source spans
* Offset-to-line/column mapping
* Source-file loading and error handling
* Command-line compiler driver
* Token representation and token-kind diagnostics
* Lexer with:

  * identifiers
  * integer literals
  * reserved keywords
  * arithmetic operators
  * comparison operators
  * logical operators
  * punctuation
  * `//` line comments
  * whitespace and trivia handling
  * source spans
  * lexical error reporting
* Automated testing with CTest and GoogleTest

## Planned Compiler Pipeline

The intended development order is:

1. Parser and AST
2. Semantic analysis and type checking
3. Reference interpreter
4. Simple non-SSA intermediate representation
5. IR verifier
6. Control-flow graph and dataflow analysis
7. Initial optimization passes
8. SSA construction where useful
9. Machine-oriented lowering
10. x86-64 instruction selection
11. Liveness analysis and register allocation
12. System V x86-64 ABI and stack-frame construction
13. Native assembly generation
14. Generated-code and performance analysis

The goal is to keep each compiler stage explicit enough to inspect, test, and reason about independently.

## Language

The language is intentionally small and statically typed.

Planned core features include:

* `i64`
* `bool`
* `void`
* functions
* local variables
* expressions
* conditionals
* loops
* function calls
* recursion

## Building

Requirements:

* C++20 compiler
* CMake 3.20+
* Ninja

Configure and build:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

## Testing

Run the test suite with:

```bash
ctest --test-dir build --output-on-failure
```

Lexer tests use GoogleTest and are discovered through CTest.

## Project Structure

```text
include/compiler/
    lexer/
    source/

src/
    driver/
    lexer/
    source/

tests/
```

## Development Direction

This project is intended to remain a standalone compiler rather than a thin frontend over an existing compiler framework.

The frontend will be followed by explicit semantic analysis, a compiler-owned IR, verification, control-flow and dataflow infrastructure, optimization, and progressively more machine-oriented lowering.

Later backend work will focus on:

* explicit machine-oriented IR and lowering
* instruction selection
* liveness analysis
* register allocation and spilling
* modest instruction scheduling
* System V x86-64 ABI handling
* stack-frame construction
* assembly generation
* symbols, object files, relocations, linking, and loading
* generated-code analysis
* performance comparison against appropriate Clang/GCC baselines
* hardware- and ISA-aware optimization reasoning

LLVM and MLIR work is planned only after the standalone implementation becomes substantial enough to provide a strong first-principles compiler foundation.
