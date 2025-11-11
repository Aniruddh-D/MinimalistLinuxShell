# Minimalist Linux Shell

A simple, lightweight shell implementation in C with support for I/O redirection and pipes.

## Overview

This project implements a functional command-line shell in C, similar to bash or zsh but with a minimalist feature set. The shell supports basic command execution, built-in commands, I/O redirection, and command piping.

## Features

- **Command Execution**: Run external programs and commands
- **Built-in Commands**:
  - `cd`: Change directory
  - `pwd`: Print working directory
  - `echo`: Print text to standard output
  - `help`: Display help information
  - `exit`: Exit the shell
- **I/O Redirection**:
  - `<`: Redirect input from a file
  - `>`: Redirect output to a file (overwrite)
  - `>>`: Redirect output to a file (append)
- **Command Piping**: Chain commands using the `|` operator
- **Tokenization**: Proper handling of command arguments including quoted strings

## Project Structure

```
MinimalistLinuxShell/
├── src/
│   └── shell-modify.c    # Main source code for the shell
└── README.md             # This file
```

## Technical Implementation

The shell is implemented with the following key components:

1. **Command Line Parser**: Tokenizes input with support for quotes and special operators
2. **Built-in Command Handler**: Implements internal shell commands
3. **Execution Engine**: Handles external command execution using fork and exec
4. **Redirection Handler**: Manages file I/O redirection
5. **Pipe Handler**: Implements command piping with inter-process communication

## Building and Running

To compile the shell:

```bash
gcc -o miniShell src/shell-modify.c
```

To run the shell:

```bash
./miniShell
```

## Usage Examples

```
# Basic command execution
T-12_MiniShell> ls -la

# I/O redirection
T-12_MiniShell> cat < input.txt
T-12_MiniShell> ls > output.txt
T-12_MiniShell> echo "append this" >> output.txt

# Command piping
T-12_MiniShell> ls -la | grep ".txt"

# Using built-in commands
T-12_MiniShell> cd /home
T-12_MiniShell> pwd
T-12_MiniShell> echo Hello World
T-12_MiniShell> help
```

## Advanced Features

- **Quoted String Handling**: Properly handles quoted arguments (both single and double quotes)
- **Error Handling**: Robust error detection and reporting
- **Memory Management**: Careful allocation and deallocation of memory

## Team Members

Internship Team-12 at Sasken:
1. Aniruddh Dubey
2. Aryan
3. Aryan Saxena
4. Abhilash Dalai
5. Vishesh Kumar Bhagat
6. Smritiparna Mahanty

## Dependencies

- Standard C library
- POSIX-compliant operating system

## Limitations

- Does not support command history or command editing
- Limited job control functionality
- No support for shell scripting

## License

This project is distributed under the MIT License. See the LICENSE file for more information.
