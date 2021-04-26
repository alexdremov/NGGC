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

## Optimization features

### Registers planning&management

Why use memory when you can use registers only?

This code does not use memory access at all. Planner maps registers to variables and saves values to RAM only when all registers are exhausted.

```python
def giveYouUp() {
	let i = 0;
	let res = 0;
	while (i < 100) {
		res += i * i;
		i += 1;
	}
	print res;
}
```

```asm
0:  55                      push   rbp
1:  90                      nop
2:  48 89 e5                mov    rbp,rsp
5:  48 81 ec 10 00 00 00    sub    rsp,0x10
c:  b8 00 00 00 00          mov    eax,0x0
11: 49 89 c3                mov    r11,rax
14: b8 00 00 00 00          mov    eax,0x0
19: 48 89 c1                mov    rcx,rax
1c: 4c 89 d8                mov    rax,r11
1f: 49 89 c2                mov    r10,rax
22: b8 64 00 00 00          mov    eax,0x64
27: 49 89 c1                mov    r9,rax
2a: 48 31 c0                xor    rax,rax
2d: 4d 39 ca                cmp    r10,r9
30: 7d 05                   jge    0x37
32: b8 01 00 00 00          mov    eax,0x1
37: 48 85 c0                test   rax,rax
3a: 0f 84 23 00 00 00       je     0x63
40: 4c 89 d8                mov    rax,r11
43: 49 89 c2                mov    r10,rax
46: 4c 89 d8                mov    rax,r11
49: 49 0f af c2             imul   rax,r10
4d: 49 89 c2                mov    r10,rax
50: 4c 01 d1                add    rcx,r10
53: b8 01 00 00 00          mov    eax,0x1
58: 49 89 c2                mov    r10,rax
5b: 4d 01 d3                add    r11,r10
5e: e9 b9 ff ff ff          jmp    0x1c
63: 48 89 c8                mov    rax,rcx
66: 48 89 c7                mov    rdi,rax
69: 48 89 8d f0 ff ff ff    mov    QWORD PTR [rbp-0x10],rcx
70: 4c 89 9d f8 ff ff ff    mov    QWORD PTR [rbp-0x8],r11
77: e8 00 00 00 00          call   0x7c
7c: 48 81 c4 10 00 00 00    add    rsp,0x10
83: 5d                      pop    rbp
84: 90                      nop
85: c3                      ret
```

### In-loop variables usage optimizations
Before entering a loop, compiler checks for used variables and force-allocate registers for them. Thus, in-loop memory access is minimal.


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
