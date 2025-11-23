# ABC: A Better C

**ABC** was designed as a modern educational programming language, continuing
the teaching philosophy of **Pascal**, but with the practical relevance of
**C**.

Pascal was once an ideal language to **learn programming from first
principles**. It allowed students to truly understand and implement fundamental
concepts, such as:

- variables and types
- control flow and procedures
- memory organization
- stack and heap behavior

After learning Pascal, students were typically able to pick up other
programming languages quickly and confidently, because they had developed a
solid understanding of **how software is fundamentally executed**.

Today, many modern languages (such as Python) abstract away too many of these
details, giving learners an incomplete mental model of what is really happening
"under the hood." At the other extreme, languages like C expose all the
low-level mechanisms — but suffer from **syntactic pitfalls** and **semantic
inconsistencies** that make them unnecessarily difficult to learn, especially
for beginners.

For example:

- complex declaration syntax (`int *p[10]` vs `int (*p)[10]`)
- inconsistent treatment of arrays and structs in function calls
- subtle pointer behavior

ABC is intentionally designed to address these issues:

- The **declaration syntax** is inspired by Pascal, making it more readable and
  consistent.
- **Semantic inconsistencies** from C are deliberately cleaned up, thanks to
  the freedom of not requiring backwards compatibility.
- ABC offers a clear, consistent model of memory and data handling — allowing
  students to experience and understand the core problems that modern languages
  are designed to solve (e.g. memory management, ownership, aliasing).

Understanding these problems is critical:

You can only fully appreciate advanced solutions like Rust's **Borrow
Checker**, reference counting, or garbage collection if you first understand
the underlying challenges. ABC enables students to encounter these issues
consciously, in a clean and accessible language.

In particular, ABC helps learners understand:

- global, local, and static variables
- stack vs. dynamic memory vs. global data
- parameter passing mechanisms
- memory layout of structs, arrays, and pointers
- explicit memory management
- how software is executed on the machine

ABC is therefore intended as a **modern didactic programming language** — not
only for teaching *how to program*, but also for teaching *how computers
execute programs*. It is suitable for use in:

- introductory programming courses
- systems programming courses
- high-performance computing courses
- compiler construction courses
- and as a foundation for understanding modern language features.

## Installation

Verify that the correct LLVM version is used:

```bash
llvm-config --version
# should print 21.x (or higher)
```

Clone the repository, build, and install:

```bash
git clone https://github.com/michael-lehn/abc-llvm.git
cd abc-llvm
make
sudo make install
```

If needed, you can explicitly specify the C++ compiler and `llvm-config` when
invoking `make`, for example:

```bash
make CXX=g++ llvm-config=llvm-config-21
```

### Hints on Installing LLVM

ABC requires **LLVM 21** (including `llvm-config` and `clang`).  
Earlier LLVM versions will **not** work.

**macOS (recommended):**

```bash
brew install llvm@21
```

After installation, make sure the Homebrew LLVM tools are in your `PATH`:

```bash
export PATH="$(brew --prefix llvm@21)/bin:$PATH"
```

**Debian/Ubuntu-based Linux:**  
Packages are available via the official LLVM APT repository:  
https://apt.llvm.org

**Arch Linux:**

```bash
sudo pacman -S llvm clang
```

**Fedora / RHEL / CentOS:**

```bash
sudo dnf install llvm clang
```

**Windows:**  
Use the official LLVM installer or build from source:  
https://llvm.org


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
  
Occasionally, we need to specify that the data pointed to by a pointer should
remain unchanged, or that the pointer itself should remain fixed, indicating
that it shouldn't be redirected. In some cases, both conditions apply: neither
the pointer nor the data it points to should change. Here are examples of such
declarations in C:

```c
    const int *c[10];
    int (* const d)[10];
    const int (* const e)[10];
```
It's important to note that in C, using `const` doesn't guarantee that the
variable is immutable. With an appropriate cast, the content of a variable can
still be altered. In ABC, the keyword `readonly` is used for this purpose. It
signifies that while technically it's still possible to modify the value (if
one really insists), the declaration clearly states the intent to access it in
a read-only manner:

```abc
    c: array[10] of -> readonly int;
    d: array[10] of readonly -> int;
    e: array[10] of readonly -> readonly int;
```
Here, we have declared

- `c` as an array of 10 elements, where each element is a pointer to a readonly integer
- `d` as an array of 10 elements, where each element is a readonly pointer to an integer
- `e` as an array of 10 elements, where each element is a readonly pointer to a readonly integer

But that's not all about pointers. A function name represents an address, the
address of its first instruction. Hence you can store the function address in a
pointer variable. Such a pointer is then called a function pointer. Here, a
declaration of a local or global pointer variable to a function that has no
return type and does not accept any parameters:

```c
void (*f)(void);
```
And here an example where `f` gets initialized such that it points to a
function `foo` by simply assigning the function name:

```c
void (*f)(void) = foo;
```
For function parameters, another syntax can be used which is more expressive.
For example, here

```c
void someFunction(void f(void));
```
function `someFunction` has a parameter `f` which is a function pointer with
the same type declared above.

In ABC, you just have one way to declare such a function pointer:
```abc
f: -> fn();
```
Hence the declaration of `someFunction` becomes
```abc
fn someFunction(f: -> fn());
```

Let us consider some more exciting examples:

- With `g: -> fn(:int)` one declares a pointer to a function that has one
  parameter of type `int` and has no return type. Optionally you can use
  parameter names for readability which the compiler ignores. Hence `g: ->
  fn(value :int)` would be equivalent.
- With `h: -> fn(:int):int` or `h: -> fn(value :int):int` one declares a
  pointer to a function with one integer parameter and an integer return type.

Got the idea? Then you might already guess that
```abc
foo: -> fn(sel: int, value: int): -> fn(value: int): -> int;
```
declares a function pointer `foo` to a function with two integer parameters
which returns a pointer to a function that has one integer parameter and
returns a pointer to an integer. Now declare this in C without typedefs ;-)

# Examples

```c
@ <stdio.hdr>

fn main()
{
    printf("hello, world!\n");
}
```

The worst part of C is the  C preprocessor (CPP). Hence it is greate that new C
like languages are avoiding the preprocessor. But because ABC is just "A Better
C, but still C" it does have a preprocessor. Students need to be prepared for
this ugly side of C. However, compared to CPP the preprocessor has limited
features. It can be used to include header files and you can define some simple
macros. 

Of course, the header file `stdio.hdr` does not contain the implementation of
`printf` but just a declaration for it. From the preprocessor the compiler gets
the following code:

```c
extern fn printf(fmt: -> char, ...);

fn main()
{
    printf("hello, world!\n");
}
```
Compared to using CPP no include guards are required when the ABC preprocessor
is used. Every file gets included only once (like using `@pragma once` with
CPPs that support this pragma). 

For teaching purposes (i.e. for showing the ugly side of C), consider this
example: 

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

Of course this triggers an error from the compiler. But it is hard to see from
the error message the actual problem:

```
    local X: int = 42;
          ^^
macro.abc:5.11-5.12: error: expected local variable declaration list
```

Sure, the error message actually could show the code the compiler got from the
preprocessor. But using the preprocessor should not be attractive. If you want
to use symbols for literals use languages features, e.g. enum constants or
constant expressions. Intead of macro functions use inline functions. Don't use
a preprocessor.

# Language Description

## Lexical Elements

Comments can be delimited by `/*` and `*/`. Nested comments are not supported.
Alternatively, comments start with `//` and are ended by the next line
terminator. Comments are treated like space characters.

Each program source is converted into a sequence of tokens during lexical
analysis. Tokens can be punctuators consisting of one or more special
characters, reserved keywords, identifiers, or literals. Tokens end if the next
character is a space or if the next character cannot be added to it.

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
`goto`      `if`        `label`     `local`     `nullptr`
`of`        `return`    `sizeof`    `struct`    `switch`
`then`      `type`      `union`     `while`

Identifiers begin with a letter, i.e., `A` to `Z` and `a` to `z`, or an
underscore `_`, and are optionally followed by a sequence of more letters,
underscores, or decimal digits `0` to `9`. Some identifiers are predefined.

The following predefined identifiers are used as named
[types](#types) (essentially keywords):

`void`              `bool`              `u8`                `u16`
`u32`               `u64`               `i8`                `i16`
`i32`               `i64`               `int`               `long`
`long_long`         `unsigned`          `unsigned_long`     `unsigned_long_long`
`size_t`            `ptrdiff_t`         `float`             `double`

The following predefined identifiers are used as named constants:

`nullptr`

Literals can be decimal literals, octal literals, hexadecimal literals, string
literals, character literals, and (decimal) floating point literals.
- Decimal literals begin with a digit `1` to `9` and optionally have more
  digits from `0` to `9`. Decimal constants are unsigned and can be of
  arbitrary size.
- Octal literals begin with a digit `0` and optionally have more digits from
  `0` to `7`.
- Hexadecimal literals begin with a prefix `0x` and one or more digits from `0`
  to `9`, 'a' to 'f', or 'A' to 'F'.
- String literals are delimited by `"`. Backslashes, i.e., `\`, are escape
  characters, removing the special meaning of the following character or
  allowing the insertion of special characters into a string.
- Character literals are delimited by `'` and consist of a single character
  (which can be an escaped character).
- Currently only decimal (but not hexadecimal) floating point literals are
  supported (see
  [floating_literal](https://en.cppreference.com/w/cpp/language/floating_literal))

## Expressions

The syntax for expressions is very similar to expressions in C, with the following exceptions:
- Expression lists are currently not implemented.
- Not all operators are currently supported. On the to-do list are bitwise
  operators, the `alignas` operator, and the `alignof` operator.
- For a conditional expression, the ternary Operator from C can be used, e.g.,
  `x = a > b ? y : z`, or alternatively, the more verbose notation `x = a > b
  then y else z`. At least currently, "?" and "then", and ":" and "else" are
  interchangeable. That means `x = a > b ? y else z` and `x = a > b then y : z`
  are also alternatives.
- For type casts, the C syntax is *not* supported. Instead, the C++ style
  notation `x = int(y)` is used for casting the expression `y` to type `int`.
- Pointers can be dereferenced like in C with the prefix operator `*`.
  Additionally, the arrow operator `->` can be used; i.e., the use of the
  operator `->` is not restricted to "struct pointers". Hence, if `x` is a
  pointer to `int`, the expressions `*x = 42` and `x-> = 42` are equivalent,
  and in both cases, `42` is assigned to the integer at the end of pointer `x`.

The EBNF grammar for expressions is:

```ebnf
          expression-list = assignment-expression { "," assignment-expression}
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
                          | postfix-expression "[" expression-list "]"
                          | postfix-expression "(" expression-list ")" 
                          | postfix-expression "++" 
                          | postfix-expression "--"
       primary-expression = identifier
                          | "sizeof" "(" (type | expression-list) ")"
                          | "nullptr"
                          | decimal-literal
                          | octal-literal
                          | hexadecimal-literal
```


```ebnf
            compound-expression = string-literal
                                | "{" compound-expression-initializer { "," compound-expression-initializer } "}"
compound-expression-initializer = [designator "=" ] assignment-expression
                     designator = "." identifier"
                                | "[" assignment-expression "]" 
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
           named-type = identifier
         pointer-type = "->" type
           array-type = "array" array-dim-and-type
   array-dim-and-type = "[" assignment-expression "]" { "[" assignment-expression "]" } "of" type
        function-type = "fn" [identifier] "(" function-parameter-list ")" [ ":" type ]
```

## Structure of an ABC Program

```ebnf
input-sequence        = {top-level-declaration} EOI
top-level-declaration = function-declaration-or-definition
                      | extern-declaration
                      | global-variable-definition
                      | type-declaration
                      | enum-declaration
                      | struct-declaration
```

#### Function Declarations and Definitions

```ebnf
function-declaration-or-definition = function-header (";" | function-body)
                   function-header = "fn" identifier "(" function-parameter-list ")" [ ":" type ]
           function-parameter-list = [ [identifier] ":" type { "," [identifier] ":" type} } ["," "..."] ]
                     function-body = compound-statement
```

```ebnf
         extern-declaration = "extern" ( function-declaration | extern-variable-declaration ) ";"
       function-declaration = function-type
extern-variable-declaration = identifier-list ":" type { "," identifier-list ":" type }
            identifier-list = identifier { "," identifier }

```

#### Global Variable Declarations and Definitions

```ebnf
global-variable-definition = "global" variable-definition-list ";"
  variable-definition-list = variable-definition { "," variable-definition }
       variable-definition = identifier-list ":" type
                                [ "=" initializer-expression ]
    initializer-expression = compound-expression
                           | assignment-expression
```

#### Type Aliases

```ebnf
type-declaration = "type" identifier ":" type ";"
```

#### Structured Type Declaration

```ebnf
       struct-declaration = "struct" identifier (";" | struct-member-declaration )
struct-member-declaration = "{" { ( "union" "{" struct-member-list "}"| struct-member-list) } "}" ";"
       struct-member-list = identifier { "," identifier } ":" ( type | struct-declaration ) ";"
```

#### Enumeration Type Declaration and Enumeration Constants

```ebnf
  enum-declaration = "enum" identifier ":" integer-type "{" { enum-constant-list } "}" ";"
enum-constant-list = identifier [ "=" assignment-expression] { "," identifier [ "=" assignment-expression] }
```

### Statements

#### Compound Statements

```ebnf
            compound-statement = "{" { statement-or-declaration-list } "}"
 statement-or-declaration-list = "{" { statement | declaration } "}"
                     statement = compound-statement
                               | if-statement
                               | switch-statement
                               | while-statement
                               | do-while-statement
                               | for-statement
                               | return-statement
                               | break-statement
                               | continue-statement
                               | goto-statement
                               | label-definition
                               | expression-statement
                   declaration = type-declaration
                               | enum-declaration
                               | struct-declaration
                               | static-variable-definition
                               | local-variable-definition
    static-variable-definition = "static" variable-definition-list ";"
     local-variable-definition = "local" variable-definition-list ";"
```

#### Expression Statements

```ebnf
expression-statement = [expression-list] ";"
```

#### Control Structures

##### If-then-(else) statements

```ebnf
if-statement      = "if" "(" expression-list ")" compound-statement
                     [ "else" if-statement | compound-statement ]
```

##### Switch statements

```ebnf
        switch-statement = "switch" "(" expression-list ")" "{" switch-case-or-statement "}"
switch-case-or-statement = "case" expression-list ":"
                         | "default" ":"
                         | statement
```

##### Loops

###### While Loops

```ebnf
while-statement = "while" "(" expression-list ")" compound-statement
```

###### Do-While Loops

```ebnf
do-while-statement = do compound-statement "while" "(" expression-list ")" ";"
```

###### For Loops

```ebnf
for-statement = "for" "(" [expression-or-local-variable-definition]
                          [expression-list] ";" [expression-list] ")"
                          compound-statement
expression-or-local-variable-definition = expression-list ";"
                                        | local-variable-definition
```

###### Break and continue 

```ebnf
return-statement = "return" [ expression-list ] ";"
```

```ebnf
break-statement = "break" ";"
continue-statement = "continue" ";"
```

###### Goto and labels

```ebnf
  goto-statement = "goto" identifier ";"
label-definition = "label" identifier ":"
```
