# Pudel

Pudel (read as poodle) is a simple interpreted programming language written in C, based on the tree-walk interpreter. It was created as a learning project to better understand how programming languages and interpreters work.

This project is currently on hold due to increasing complexity caused by static typing combined with AST walker, however I may return to development after playing around with the VM.

Pudel is inspired by `jlox` from ["Crafting Interpreters" by Robert Nystrom](https://craftinginterpreters.com/), but with own modifications, like static typing and more assignment operators.

## Features

- Support for `int`, `float` and `bool` variables
- Arithmetic operations: `+`, `-`, `*`, `/`, `%`
- Assignment: `=`, `+=`, `-=`, `*=`, `/=`, `%=`
- Comparison operators: `==`, `!=`, `<`, `<=`, `>`, `>=`
- Logical operators: `&&`, `||`
- Conditional statements: `if`, `else`
- Loops: `while`, `for`

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
