# NGGC
Compilable to Mach-O Never Gonna Give you up© programming language

## Features

- x86_64 compilable
- Object files in Mach-O format
- Call external functions

**plans**

- Full System V support -> call any C or C++ function

## Requirment

Requires MachOBuilder to be installed: https://github.com/AlexRoar/MachOBuilder

## Installation

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j3
make install -j3
```

## Examples

```python
def giveYouUp() {
	let x = 0;
	let userData = input;
	while(x < userData) {
	    print fact(x);
	    x += 1;
	}
}

def fact(x) {
    if (x <= 1){ ret 1; }
    
    ret fact(x - 1) * x;
}
```

## Usage

```bash
NGGC help
--input        <input file> input file to be compiled .ngg format (source)
-o, --output   <output file> output file. a.out by default (mach-o executable)
-h, --help     show this help message
-c             object file only
-C             keep object file
--verbose      output debug information to the console
--lex          <.lex file> file to dump lexemes
--lst          <.lst file> file to dump detailed listing
-g             generate AST graph
-d             dump AST graph
```

## Structure
### Main function: 
giveYouUp() -> main()

### Function definition:
```python
def identifier(a, b, c, ...) {
  ...
}
```

Return value: ret ... ;
Every function returns some value even if it is unspecified.
All local variables are mutable.
All functions can be accessed from extern object files.

### Operations:
\*, -, +, / (int division), <, >, <=, >=, !=, ==,

### Var declaration
Variables must be declared before their first use.
```js
let name = 123;
let something;
```

### Polymorphysm 
Functions can have the same name and different number of arguments in the same file.
```python
def poly(a) {
  print a;
}
```
```python
def poly(a, b) {
  print a * b;
}
```
