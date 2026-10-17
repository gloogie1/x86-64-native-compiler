# x86-64 Native Compiler

A standalone C++20 compiler for a small statically typed language, built from first principles with the goal of generating native x86-64 code.

The compiler uses a hand-written frontend and is being developed through the full compilation pipeline: parsing, semantic analysis, compiler-owned IR, optimization, machine lowering, register allocation, and x86-64 code generation.

The current implementation includes the **source infrastructure, lexer, AST, and recursive-descent parser**. Semantic analysis is the next major stage.

## Compiler Pipeline

```text
Source
  |
  v
Lexer
  |
  v
Token stream
  |
  v
Parser
  |
  v
AST
  |
  v
Semantic analysis       <- next
  |
  v
Compiler IR
  |
  v
CFG / dataflow analysis
  |
  v
Optimization
  |
  v
Machine lowering
  |
  v
Register allocation
  |
  v
x86-64 code generation
```

The project intentionally keeps the major compiler stages explicit rather than delegating the pipeline to an existing compiler framework.

## Technical Highlights

### Frontend

- Hand-written lexer and recursive-descent parser
- Byte-offset source spans retained through lexical and syntactic analysis
- Offset-to-line/column mapping for source diagnostics
- Explicit operator precedence and associativity in expression parsing
- Structured lexical and parse errors
- AST ownership expressed using C++ smart pointers
- `std::variant`-based result/error representation
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

This allows tokens and parse errors to retain their connection to the input source while keeping the internal representation compact. Source offsets can be translated to line and column locations when diagnostics are presented to the user.

## Language

The language is intentionally small so that the complete compiler pipeline can remain understandable and independently implementable.

### Types

```text
i64
bool
void
```

### Core constructs

- functions
- parameters and local variables
- arithmetic expressions
- comparison expressions
- boolean expressions
- function calls
- `if` / `else`
- `while`
- `return`
- recursion

A representative program is intended to look like:

```text
fn factorial(n: i64) -> i64 {
    if (n <= 1) {
        return 1;
    }

    return n * factorial(n - 1);
}
```

The language is deliberately constrained: the focus of the project is the compiler architecture and backend rather than designing a large programming language.

## Current Status

### Implemented

**Source infrastructure**
- source-file loading and error handling
- source offsets and spans
- line/column mapping
- command-line compiler driver

**Lexical analysis**
- identifiers and reserved keywords
- integer literals
- arithmetic operators
- comparison operators
- logical operators
- delimiters and punctuation
- whitespace and `//` comments
- source-aware lexical errors

**Abstract syntax tree**
- primitive type representation
- literal expressions
- variable expressions
- unary and binary expressions
- function calls
- statements and declarations
- functions and control-flow constructs
- explicit tree ownership

**Parsing**
- recursive-descent parsing
- precedence-aware expression parsing
- function and statement parsing
- AST construction
- source-aware parse errors

Parser test coverage is currently being expanded before beginning semantic analysis.

## Development Roadmap

The project is being developed incrementally so that each stage can be tested and reasoned about independently.

- [x] Source infrastructure
- [x] Lexer
- [x] AST
- [x] Recursive-descent parser
- [ ] Complete parser test coverage
- [ ] Semantic analysis and type checking
- [ ] Reference interpreter
- [ ] Non-SSA compiler IR
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

Later stages will explore:

- machine-oriented IR and lowering
- instruction selection
- liveness analysis
- register allocation and spilling
- modest instruction scheduling
- System V AMD64 calling convention
- stack-frame construction
- assembly generation
- symbols, object files, relocations, linking, and loading
- generated-code analysis
- comparison against appropriate GCC/Clang baselines
- hardware- and ISA-aware optimization

LLVM and MLIR experimentation is intentionally deferred until the standalone compiler is sufficiently developed to provide a first-principles foundation for comparing compiler architectures.

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

## Project Goals

This project is intended to exercise the parts of compiler engineering that become important beyond basic parsing:

- semantic analysis and language invariants
- intermediate representation design
- control-flow and dataflow analysis
- optimization
- machine-level lowering
- instruction selection
- register allocation
- ABI implementation
- native code generation
- generated-code quality analysis

The long-term goal is a compiler where the path from source program to native x86-64 code remains explicit enough to inspect, test, measure, and reason about at every stage.