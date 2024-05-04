# ABC: A Bloody Compiler for A Better C

The ABC programming language is meticulously crafted as a teaching tool,
specifically designed to impart a deep understanding of C programming while
prioritizing teachability.

Beyond serving as a vehicle for mastering C, ABC acts as a springboard for
exploring why C (or ABC) might not always be the optimal choice for a project,
and for delving into alternative languages better suited to specific
requirements. Proficiency in ABC equips learners with the foundational
programming skills necessary to readily adapt to these alternatives.

## Better in what respect?

In the realm of C instruction, challenges surrounding pointers and memory
management are paramount, whether grappling with global, local, or dynamic
memory. Traditional C's syntactic complexities and semantic inconsistencies
often obscure fundamental concepts, impeding student comprehension.

ABC tackles these challenges head-on by adopting a Pascal-like syntax for
declarations and maintaining semantic coherence throughout the language. By
simplifying declaration syntax and ensuring consistency, ABC minimizes
distractions, enabling students to focus on mastering memory management and
pointer manipulation principles. In this capacity, ABC transcends being just a
programming language; it becomes a catalyst for educators to deliver a more
effective and engaging learning experience.

With its lucid syntax and coherent semantics, ABC strives to be more than a
mere teaching language—it aspires to be *A Better C*, empowering students to
navigate C programming's intricacies with confidence and clarity.

## Why C (or ABC) and not ...?

The last decade has witnessed the emergence of numerous programming
languages—like Rust, Zig, Go, Hare,and Odin—each positioned as a superior
alternative to C (or C++). These languages address specific C programming
challenges and offer distinct solutions, prioritizing different aspects of
programming.

Some emphasize memory management, incorporating features like garbage collection,
borrow checker, or linear types, while others prioritize performance, eschewing
such mechanisms. Variations in handling unsafe pointers also reflect differing
language priorities.

However, discussions about garbage collectors and safe pointers may seem
abstract without practical experience. In C (and consequently, ABC), learners
confront the very problems these features solve, laying a solid foundation for
understanding and appreciating design decisions in alternative languages.

## How to use ABC to learn programming?

Programming intertwines with hardware and tooling like compilers, and often
spans multiple domains, including operating systems. For C-like languages,
understanding these connections is crucial and can be achieved through two main
avenues:

- Building hardware: Starting with logic gates, learners progress to building
  their own computers using essentials like Field Programmable Gate Arrays
  (FPGAs).

- Writing a simple compiler: A tutorial will be provided to guide learners
  through building their own compiler from scratch, facilitating understanding
  of key programming concepts.

These approaches may initially seem daunting, but with focused attention on
essentials, learners can effectively navigate and master them. A comprehensive
stack of learning materials is available to support learners in these
endeavors.

# How ABC addresses problems in teaching C

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
  
Of course, sometimes we need to express that the data at the end of the pointer is constant, or that the pointer itself is constant, meaning the pointer cannot be redirected. Sometimes, both conditions apply — that the pointer itself is constant and the data at the end. Here are some examples of such declarations in C:

```c
    const int *c[10];
    int (* const d)[10];
    const int (* const e)[10];
```
and the equivalent declarations in ABC:

```abc
    c: array[10] of -> const int;
    d: array[10] of const -> int;
    e: array[10] of const -> const int;
```
Here, we have declared

- `c` as an array of 10 elements, where each element is a pointer to a constant integer
- `d` as an array of 10 elements, where each element is a constant pointer to an integer
- `e` as an array of 10 elements, where each element is a constant pointer to a constant integer

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
`size_t`            `ptrdiff_t`

The following predefined identifiers are used as named constants:

`nullptr`

Literals can be decimal literals, octal literals, hexadecimal literals, string literals, and character literals.
- Decimal literals begin with a digit `1` to `9` and optionally have more digits from `0` to `9`. Decimal constants are unsigned and can be of arbitrary size.
- Octal literals begin with a digit `0` and optionally have more digits from `0` to `7`.
- Hexadecimal literals begin with a prefix `0x` and one or more digits from `0` to `9`, 'a' to 'f', or 'A' to 'F'.
- String literals are delimited by `"`. Backslashes, i.e., `\`, are escape characters, removing the special meaning of the following character or allowing the insertion of special characters into a string.
- Character literals are delimited by `'` and consist of a single character (which can be an escaped character).

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

