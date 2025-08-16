# Pudel

Pudel (read as poodle) is a simple interpreted programming language written in C, based on the tree-walk interpreter. It was created as a learning project to better understand how programming languages and interpreters work.

Pudel is inspired by `jlox` from ["Crafting Interpreters" by Robert Nystrom](https://craftinginterpreters.com/).

## Features

- Arithmetic operations: `+`, `-`, `*`, `/`
- Assignment: `=`, `+=`, `-=`, `*=`, `/=`
- Comparison operators: `==`, `!=`, `<`, `<=`, `>`, `>=`
- Logical operators: `and`, `or`
- Conditional statements: `if`, `else`
- Ternary conditional operator: `?:`
- Loops: `while`, `for`
- Dynamic variables with types: `number`, `bool`, `string`, `list`
- Native functions: `print`, `input`, `typeof`
- Value type conversions

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
