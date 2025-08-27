# Pudel

Pudel (read as poodle) is a simple interpreted programming language written in C, based on the tree-walk interpreter. It was created as a learning project to better understand how programming languages and interpreters work. It's syntax is somewhat a cross between `C` and `Python`.

Pudel is inspired by `jlox` from ["Crafting Interpreters" by Robert Nystrom](https://craftinginterpreters.com/).

Interpreter has some of the optimizations, like:
- lexing one token at a time (less memory usage)
- recognizing keywords using trie instead of simple loop (faster keyword/identifier recognition)
- using distinct AST nodes instead of union (less memory usage)
- string interning (especially effective during interpreting recursive functions)

## Features

- Arithmetic operations: `+`, `-`, `*`, `/`
- Assignment: `=`, `+=`, `-=`, `*=`, `/=`
- Comparison operators: `==`, `!=`, `<`, `<=`, `>`, `>=`
- Logical operators: `and`, `or`
- Conditional statements: `if`, `else`
- Ternary conditional operator: `?:`
- Loops: `while`, `for`
- Dynamic variables with types: `int`, `float`, `bool`, `string`, `list`
- Native functions: `print`, `input`, `typeof`, `clock`
- Explicit value type conversions, e.g. `int(10.45)`
- Implicit value type promotion in arithmetic operations, allowing operations like `true * (10 + 3.6)`
- User functions

## Building

To build the interpreter, run:

```bash
make all
```

## Running a Program

Pudel currently runs only source files passed as command-line argument:

```bash
./pudel examples/factorial.pud
```
