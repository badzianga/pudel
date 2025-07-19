# Pudel

Pudel (read as poodle) is a work-in-progress simple interpreted programming language written in C, based on the tree-walk interpreter. It was created as a learning project to better understand how programming languages and interpreters work. The syntax of Pudel is currently identical to C, but there will be differences in the future.

This project is inspired by ["Crafting Interpreters" by Robert Nystrom](https://craftinginterpreters.com/), but with own modifications.

## Features

- Support for `int` variables
- Arithmetic operations: `+`, `-`, `*`, `/` and `%`
- Assignment: `=`, `+=`, `-=`, `*=`, `/=` and `%=`
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

## Future improvements

- Additional data types: `bool`, `float`, `string`
- User-defined functions
- Internal functions
- Classes and basic object-oriented features
- Interactive REPL
- Improved error handling and diagnostics
- `break` and `continue` in loops
