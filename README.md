# NGGC
Compilable to Mach-O Never Gonna Give you up© programming language

## Requirment

Requires MachOBuilder to be installed: https://github.com/AlexRoar/MachOBuilder

## Features

- x86_64 compilable
- Object files in Mach-O format
- Call external functions

**plans**

- Full System V support -> call any C or C++ function


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
