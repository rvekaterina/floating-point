# Floating point arithmetic implementation

A C implementation of IEEE 754 floating-point operations supporting both 32-bit (float) and 16-bit (half) precision.

## Features

- Supports basic arithmetic operations: addition, subtraction, multiplication, division
- Implements four rounding modes:
    - 0: Toward zero
    - 1: To nearest (ties to even)
    - 2: Toward +∞
    - 3: Toward -∞
- Prints the result in the same format as the `%a` format specifier

## Build

```bash
make
```

## Usage

### Command line interface
```bash
./float <mode (f|h)> <rounding> <hex_value1> [<operation> <hex_value2>]