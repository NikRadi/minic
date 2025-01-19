# minic
minic is a small C-compiler project inspired by [chibicc](https://github.com/rui314/chibicc) and [acwj](https://github.com/DoctorWkt/acwj).


### Architecture
minic has the following compilation stages:  
1. Parse: Break the file into tokens and create an abstract syntax tree (AST).
2. Analyze: Resolve variable and function names, check types, and evaluate expressions like `sizeof`.
3. Code generation: Generate NASM-compatible assembly targeting x86_64 architecture.
