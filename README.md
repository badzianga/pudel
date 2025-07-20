# Pudel

Pudel (read as poodle) is a work-in-progress simple interpreted programming language written in C, based on the tree-walk interpreter. It was created as a learning project to better understand how programming languages and interpreters work.

This project is inspired by ["Crafting Interpreters" by Robert Nystrom](https://craftinginterpreters.com/), but with own modifications, like static typing and more assignment operators.

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

## Future improvements

- Proper variable scopes
- Strings
- User-defined functions
- Native functions
- Classes and basic object-oriented features
- Hash maps for variables, functions and classes lookup
- Interactive REPL
- Improved error handling and diagnostics
- `break` and `continue` in loops
- Ternary operator
- Multi-line comments
- Multiple statements in one line, separated by `,`
- Bitwise operators: `&`, `|`, `^`, `~` and their assignments
