# ABC: A Better C

ABC is a C-like language that uses LLVM for code generation and features
Pascal-inspired syntax for declarations. The language is specifically designed
for educational purposes, helping learners transition to languages like C, C++,
Rust, and Zig. Unlike Python, JavaScript, or Ruby, these languages are closer
to the hardware and require a deeper understanding of low-level programming
concepts.

## Why Another Programming Language?

What is the Easiest Programming Language Among C, C++, Rust, and Zig?

- **Simplest Syntax and Easiest to Learn**: C has the simplest syntax and is
  the easiest to learn, but it offers little safety and requires manual memory
  management.
- **Best Balance of Simplicity and Safety**: Zig offers a good balance between
  simple syntax and modern safety features.
- **Complex but Safe**: Rust is the most complex to learn, but it provides the
  best safety mechanisms and long-term benefits due to its memory and thread
  safety models.
- **Powerful but Complex**: C++ is very powerful but also very complex, with a
  steep learning curve due to the multitude of language features.

All these languages present significant challenges that can be easier to tackle
with a language specifically designed to teach these concepts. Introducing a
new C-like language that focuses on fundamental programming concepts could
provide a smoother transition to learning C, C++, Rust, and Zig. This new
language would serve as an educational stepping stone, helping learners grasp
essential principles and paradigms before moving on to more complex and
specialized languages.

### How ABC Aims to Achieve This Goal

To address these challenges, the following design decisions have been made for ABC:

- **Simplified Grammar for Expressions and Statements**: The grammar for
  expressions and statements in ABC is heavily inspired by C. With a few
  simplifications and avoidance of inconsistencies, the grammar is nearly
  identical. A simple syntax makes the language easier to learn.
- **Simplified Declaration Syntax Inspired by Pascal**: ABC adopts Pascal-like
  syntax for declarations (e.g., identifier first, then type). In C/C++,
  declarations involving pointers are particularly challenging (see examples
  below with a pointer to an array of integers and an array of pointers to
  integers). Handling pointers is a significant challenge in learning C, C++,
  Zig, and Rust, making this an important aspect.
- **Explicit Scope and Lifetime Keywords**: Understanding the scope and
  lifetime of variables is crucial. These concepts are often new to beginners,
  and the differences are not always clear initially. In ABC, variable
  declarations explicitly use the keywords `global`, `local`, or `static` to
  clarify their meanings.
- **Teaching Memory Management Concepts**: Beginners need to learn about global
  variables (static lifetime), which reside in the data segment, and local
  variables (automatic lifetime), which reside on the stack and become invalid
  when the function exits. For dynamic memory management, ABC requires the use
  of `malloc` and `free`, similar to C.

By incorporating these design choices, ABC aims to provide a robust foundation
for learners, making it easier for them to transition to more complex languages
like C, C++, Rust, and Zig. ABC serves as an educational tool that simplifies
core programming concepts while maintaining enough complexity to prepare
learners for the challenges of advanced languages.


## Declarations in C and ABC

For comparison, these declarations in C:
```c
    int *a[10];
    int (*b)[10];
```
are equivalent to these declarations in ABC:

```abc
    a: array[10] of -> int;
    b: -> array[10] of int;
```

In both cases, one declares

- `a` as an array of 10 elements, where each element is a pointer to an integer
- `b` as a pointer to an array of 10 integers
  
Occasionally, we need to specify that the data pointed to by a pointer should remain unchanged, or that the pointer itself should remain fixed, indicating that it shouldn't be redirected. In some cases, both conditions apply: neither the pointer nor the data it points to should change. Here are examples of such declarations in C:

```c
    const int *c[10];
    int (* const d)[10];
    const int (* const e)[10];
```
It's important to note that in C, using `const` doesn't guarantee that the variable is immutable. With an appropriate cast, the content of a variable can still be altered. In ABC, the keyword `readonly` is used for this purpose. It signifies that while technically it's still possible to modify the value (if one really insists), the declaration clearly states the intent to access it in a read-only manner:

```abc
    c: array[10] of -> readonly int;
    d: array[10] of readonly -> int;
    e: array[10] of readonly -> readonly int;
```
Here, we have declared

- `c` as an array of 10 elements, where each element is a pointer to a readonly integer
- `d` as an array of 10 elements, where each element is a readonly pointer to an integer
- `e` as an array of 10 elements, where each element is a readonly pointer to a readonly integer

But that's not all about pointers. A function name represents an address, the address of its first instruction. Hence you can store the function address in a pointer variable. Such a pointer is then called a function pointer. Here, a declaration of a local or global pointer variable to a function that has no return type and does not accept any parameters:
```c
void (*f)(void);
```
And here an example where `f` gets initialized such that it points to a function `foo` by simply assigning the function name:
```c
void (*f)(void) = foo;
```
For function parameters, another syntax can be used which is more expressive. For example, here
```c
void someFunction(void f(void));
```
function `someFunction` has a parameter `f` which is a function pointer with the same type declared above.

In ABC, you just have one way to declare such a function pointer:
```abc
f: -> fn();
```
Hence the declaration of `someFunction` becomes
```abc
fn someFunction(f: -> fn());
```

Let us consider some more exciting examples:

- With `g: -> fn(:int)` one declares a pointer to a function that has one parameter of type `int` and has no return type. Optionally you can use parameter names for readability which the compiler ignores. Hence `g: -> fn(value :int)` would be equivalent.
- With `h: -> fn(:int):int` or `h: -> fn(value :int):int` one declares a pointer to a function with one integer parameter and an integer return type.

Got the idea? Then you might already guess that
```abc
foo: -> fn(sel: int, value: int): -> fn(value: int): -> int;
```
declares a function pointer `foo` to a function with two integer parameters which returns a pointer to a function that has one integer parameter and returns a pointer to an integer. Now declare this in C without typedefs ;-)

# Examples

```c
@ <stdio.hdr>

fn main()
{
    printf("hello, world!\n");
}
```

The worst part of C is the  C preprocessor (CPP). Hence it is greate that new C like languages are avoiding the preprocessor. But because ABC is just "A Better C, but still C" it does have a preprocessor. Students need to be prepared for this ugly side of C. However, compared to CPP the preprocessor has limited features. It can be used to include header files and you can define some simple macros. 

Of course, the header file `stdio.hdr` does not contain the implementation of `printf` but just a declaration for it. From the preprocessor the compiler gets the following code:

```c
extern fn printf(fmt: -> char, ...);

fn main()
{
    printf("hello, world!\n");
}
```
Compared to using CPP no include guards are required when the ABC preprocessor is used. Every file gets included only once (like using `@pragma once` with CPPs that support this pragma). 

For teaching purposes (i.e. for showing the ugly side of C), consider this example: 

```c
@define X 42

fn main()
{
    local X: int = 42;
}
```

Here the ABC compiler receives the following code from the preprocessor:

```c
fn main()
{
    local 42: int = 42;
}
```

Of course this triggers an error from the compiler. But it is hard to see from the error message the actual problem:
```
    local X: int = 42;
          ^^
macro.abc:5.11-5.12: error: expected local variable declaration list
```

Sure, the error message actually could show the code the compiler got from the preprocessor. But using the preprocessor should not be attractive. If you want to use symbols for literals use languages features, e.g. enum constants or constant expressions. Intead of macro functions use inline functions. Don't use a preprocessor.

# Language Description

## Lexical Elements

Comments can be delimited by `/*` and `*/`. Nested comments are not supported. Alternatively, comments start with `//` and are ended by the next line terminator. Comments are treated like space characters.

Each program source is converted into a sequence of tokens during lexical analysis. Tokens can be punctuators consisting of one or more special characters, reserved keywords, identifiers, or literals. Tokens end if the next character is a space or if the next character cannot be added to it.

Punctuators are:

`.`         `...`       `;`         `:`         `,`         `{`
`}`         `(`         `)`         `[`         `]`         `^`
`+`         `++`        `+=`        `-`         `--`        `-=`
`->`        `*`         `*=`        `/`         `/=`        `%`
`%=`        `=`         `==`        `!`         `!=`        `>`
`>=`        `<`         `<=`        `&`         `&&`        `|`
`||`        `?`         `#`

The following keywords are reserved and cannot be used as identifiers:

`alignas`   `alignof`   `array`     `break`     `case`
`const`     `continue`  `default`   `do`        `else`
`enum`      `extern`    `fn`        `for`       `global`
`if`        `local`     `nullptr`   `of`        `return`
`sizeof`    `struct`    `switch`    `then`      `type`
`union`     `while`

Identifiers begin with a letter, i.e., `A` to `Z` and `a` to `z`, or an underscore `_`, and are optionally followed by a sequence of more letters, underscores, or decimal digits `0` to `9`. Some identifiers are predefined.

The following predefined identifiers are used as named [types](#types) (essentially keywords):

`void`              `bool`              `u8`                `u16`
`u32`               `u64`               `i8`                `i16`
`i32`               `i64`               `int`               `long`
`long_long`         `unsigned`          `unsigned_long`     `unsigned_long_long`
`size_t`            `ptrdiff_t`         `float`             `double`

The following predefined identifiers are used as named constants:

`nullptr`

Literals can be decimal literals, octal literals, hexadecimal literals, string literals, character literals, and (decimal) floating point literals.
- Decimal literals begin with a digit `1` to `9` and optionally have more digits from `0` to `9`. Decimal constants are unsigned and can be of arbitrary size.
- Octal literals begin with a digit `0` and optionally have more digits from `0` to `7`.
- Hexadecimal literals begin with a prefix `0x` and one or more digits from `0` to `9`, 'a' to 'f', or 'A' to 'F'.
- String literals are delimited by `"`. Backslashes, i.e., `\`, are escape characters, removing the special meaning of the following character or allowing the insertion of special characters into a string.
- Character literals are delimited by `'` and consist of a single character (which can be an escaped character).
- Currently only decimal (but not hexadecimal) floating point literals are supported (see [floating_literal](https://en.cppreference.com/w/cpp/language/floating_literal))

## Expressions

The syntax for expressions is very similar to expressions in C, with the following exceptions:
- Expression lists are currently not implemented.
- Not all operators are currently supported. On the to-do list are bitwise operators, the `alignas` operator, and the `alignof` operator.
- For a conditional expression, the ternary Operator from C can be used, e.g., `x = a > b ? y : z`, or alternatively, the more verbose notation `x = a > b then y else z`. At least currently, "?" and "then", and ":" and "else" are interchangeable. That means `x = a > b ? y else z` and `x = a > b then y : z` are also alternatives.
- For type casts, the C syntax is *not* supported. Instead, the C++ style notation `x = int(y)` is used for casting the expression `y` to type `int`.
- Pointers can be dereferenced like in C with the prefix operator `*`. Additionally, the arrow operator `->` can be used; i.e., the use of the operator `->` is not restricted to "struct pointers". Hence, if `x` is a pointer to `int`, the expressions `*x = 42` and `x-> = 42` are equivalent, and in both cases, `42` is assigned to the integer at the end of pointer `x`.

The EBNF grammar for expressions is:

```ebnf
               expression = assignment-expression
    assignment-expression = conditional-expression { ("=" | "+=" | "-=" | "*=" | "/=" | "%=") assignment-expression }
   conditional-expression = logical-or-expression
                                [ ("?" | "then") assignment-expression
                                  (":" | "else") conditional-expression ]
    logical-or-expression = logical-and-expression [ "||" logical-and-expression ]
   logical-and-expression = equality-expression [ "&&" equality-expression ]
      equality-expression = relational-expression [ ("==" | "!=") relational-expression ]
    relational-expression = additive-expression [ ("<" | "<=" | ">" | ">=" ) additive-expression ]
      additive-expression = multiplicative-expression [ ("+" | "-" ) multiplicative-expression ]
multiplicative-expression = unary-prefix-expression [ ("*" | "/" | "%" ) unary-prefix-expression ]
  unary-prefix-expression = ("-" | "!" | "++" | "--" | "*" | "&") unary-prefix-expression
                          | postfix-expression
       postfix-expression = primary-expression
                          | postfix-expression "." identifier
                          | postfix-expression "->" [ identifier ]
                          | postfix-expression "[" expression "]"
                          | postfix-expression "(" expression ")" 
                          | postfix-expression "++" 
                          | postfix-expression "--"
       primary-expression = identifier
                          | "sizeof" "(" (type | expression) ")"
                          | "nullptr"
                          | decimal-literal
                          | octal-literal
                          | hexadecimal-literal
```

For convenience, the precedence and associativity are summarized in the following table:

| Precedence    |   Associativity   |   Operators                           |   Meaning     |
|---------------|-------------------|---------------------------------------|---------------|
| 16 (highest) |  left | Identifier <br> Literal <br> `++` (post-increment) <br> `--` (post-decrement) <br> `f()` (function call) <br> `[i]` (index operator) <br> `->` (indirect member access or dereference operator) <br> `s.member` (direct member access)  | Primary and  Unary postfix expression |
| 15 | right | `*` (dereference operator) <br> `&` (address operator) <br> `-` (unary minus) <br>  `+` (unary plus) <br>  `!` (logical not) <br>  <br> `++` (pre-increment) <br> `--` (pre-decrement) <br> `sizeof` <br> type(expression) | Unary prefix expression |
| 13 | left | `*` (multiply) <br>  `/` (divide) <br> `%` (modulo) | Multiplicative expression |
| 12 | left | `+` (add) <br> `-` (subtract) | Additive expression |
| 10 | left | `<`  (less) <br> `>`  (greater) <br> `<=` (less equal) <br> `>=` (greater equal) | Relational expression |
| 9  | left | `==` (equal) <br> `!=` (not equal) | Equality and inequality expression |
| 5  | left | `&&` | Logical and |
| 4  | left | `\|\|` | Logical or |
| 3  | right | `?` in conjunction with `:` or `then` in conjunction with `else` | Conditional expression |
| 2  | right | `=` <br> `+=` <br> `-=` <br> `*=` <br> `/=` <br> `%=` | Assignment |

## Types

```ebnf
            type = [const] unqualified-type
unqualified-type = named-type
                 | pointer-type
                 | array-type
                 | function-type
    pointer-type = "->" type
      array-type = "array" "[" expression "]" { "[" expression "]" } "of" type
```

## Structure of an ABC Program

```ebnf
input-sequence        = {top-level-declaration} EOI
top-level-declaration = function-declaration-or-definition
                      | extern-declaration
                      | global-variable-definition
                      | type-declaration
                      | struct-declaration
                      | enum-declaration
```

#### Function Declarations and Definitions

```ebnf
function-declaration-or-definition = function-type (";" | compound-statement)
                     function-type = "fn" [identifier] "(" function-parameter-list ")" [ ":" type ]
           function-parameter-list = [ [identifier] ":" type] { "," [identifier] ":" type} } ]
```

```ebnf
  extern-declaration = "extern" ( function-declaration | variable-declaration ) ";"
function-declaration = function-type
variable-declaration = identifier ":" type
```

#### Global Variable Declarations and Definitions

```ebnf
global-variable-definition = "global" variable-declaration-list ";"
 variable-declaration-list = variable-declaration { "," variable-declaration }
      variable-declaration = identifier ":" type [ "=" initializer ]
               initializer = expression
                           | "{" initializer-list "}"
          initializer-list = [ initializer ] { "," initializer }
```

#### Type Aliases

```ebnf
type-declaration = "type" identifier ":" type ";"
```

#### Structured Type Declaration

```ebnf
       struct-declaration = "struct" identifier (";" | struct-member-declaration )
struct-member-declaration = "{" { struct-member-list } "}" ";"
       struct-member-list = identifier { "," identifier } ":" ( type | struct-declaration ) ";"
```

#### Enumeration Type Declaration and Enumeration Constants

```ebnf
  enum-declaration = "enum" identifier ":" integer-type "{" { enum-constant-list } "}" ";"
enum-constant-list = identifier [ "=" expression] { "," identifier [ "=" expression] }
```

### Statements

#### Compound Statements

```ebnf
        compound-statement = "{" { statement-or-declaration } "}"
                 statement = compound-statement
                           | if-statement
                           | switch-statement
                           | while-statement
                           | for-statement
                           | return-statement
                           | break-statement
                           | continue-statement
                           | expression-statement
               declaration = type-declaration
                           | enum-declaration
                           | struct-declaration
                           | global-variable-definition
                           | local-variable-definition
 local-variable-definition = local-variable-declaration ";"
local-variable-declaration = "local" variable-declaration-list
```

#### Expression Statements

```ebnf
expression-statement = [expression] ";"
```

#### Control Structures

##### If-then-(else) statements

```ebnf
if-statement      = "if" "(" expression ")" compound-statement [ "else" compound-statement ]
```

##### Switch statements

```ebnf
        switch-statement = "switch" "(" expression ")" "{" switch-case-or-statement "}"
switch-case-or-statement = "case" expression ":"
                         | "default" ":"
                         | statement
```

##### Loops

###### While Loops

```ebnf
while-statement = "while" "(" expression ")" compound-statement
```

###### For Loops

```ebnf
for-statement = "for" "(" [expression-or-local-variable-definition] ";"
                          [expression] ";" [expression] ")"
                          compound-statement
expression-or-local-variable-definition = expression
                                        | local-variable-declaration
```

###### Break and continue 

```ebnf
return-statement = "return" [ expression ] ";"
```

```ebnf
break-statement = "break" ";"
continue-statement = "continue" ";"
```

