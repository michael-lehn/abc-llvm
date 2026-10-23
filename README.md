## TL;DR

ABC is a small teaching language designed to make programming and computers easier to understand — not to hide how they work.

- **Understand the machine.**
  ABC exposes the basic ideas of memory, addresses, data representation, and
  how programs interact with the computer.
- **Learn the problem before the solution.**
  Many features of modern programming languages solve problems inherited from
  C. With ABC, you first encounter and understand these problems — making it
  much easier to see *why* languages such as C++, Odin, Rust, or others provide
  certain features.
- **Simpler than C, without being more magical.**
  ABC keeps the low-level programming model, but uses a smaller and more
  consistent grammar and semantics. This makes it significantly easier to learn
  than C while preserving the concepts that matter.

**Requirements:** LLVM 17 to 23 are supported and tested (see
[Installation](#installation)).

**A note on the name:** My ABC is unrelated to the ABC language developed at CWI,
which was originally called B—also unrelated to Bell Labs’ B, the predecessor of C.
My ABC shares a taste for small, close-to-the-machine languages with Bell Labs’ B,
and continues CWI’s tradition of naming a language after an unrelated one. Any
historical continuity is purely alphabetical.

## Contents

- [ABC: A Better C](#abc-a-better-c)
- [Examples](#examples)
- [Installation](#installation)
- [Raylib Examples](#raylib-examples)
- [A Compiler Written in ABC](#a-compiler-written-in-abc)
- [Language Description](#language-description)
- [Usage](#usage)


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

## Repository Layout

**Note:** This project uses hard tabs with a tab width of 8. If the code looks
misaligned on GitHub, append `?ts=8` to the URL or configure your GitHub tab
width accordingly.

### `examples/`

Contains a collection of small example programs illustrating different language
features, including

- recursion,
- pointers,
- dynamic memory allocation,
- and linked lists.

### `not-abc/`

Contains the compiler project developed as part of the undergraduate course
*Introduction to High Performance Computing*. In the course, students implement
a simple compiler for the `not-abc` language in **ABC**.

The directory has its own [not-abc/README](not-abc/README.md) describing how to
build and use the compiler.

The initial LLVM IR for the self-hosting compiler in the companion repository

https://github.com/michael-lehn/not-abc

was generated using this compiler.


## Installation

### Requirements

- LLVM 17 to 23 (see [Hints on Installing LLVM](#hints-on-installing-llvm))
- GNU Make 4.2 or newer

**macOS:** Apple's preinstalled `/usr/bin/make` is GNU Make 3.81, which is
too old. Install a recent GNU Make with Homebrew or MacPorts:

```bash
brew install make          # Homebrew
sudo port install gmake    # MacPorts
```

Both install it as `gmake`. Use `gmake` instead of `make` in all commands
below (e.g. `gmake`, `gmake check`, `sudo gmake install`).

### Building

Verify that the correct LLVM version is used:

```bash
llvm-config --version
# should print a version from 17.x to 23.x
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
make CXX=g++ llvm-config=llvm-config-23
```

Building with a different C++ compiler or a different LLVM version requires
`make clean` first, or a separate build directory, e.g.
`make build.dir=build/llvm23/ llvm-config=llvm-config-23`.

### Running the Tests

```bash
make check
```

builds the compiler and runs the test suite without installing ABC. It
consists of unit tests for compiler components (`test/unit`), ABC programs
that are compiled, checked with the LLVM verifier, and executed (`test/run`),
invalid programs that must be rejected (`test/fail`), programs concerning open
language design questions (`test/design`), checks of the LLVM IR generated for
compiler options (`test/ir`), and checks of the compilation database
(`test/tool`). Every test has a stable ID
(`T0001`, ...). Known compiler bugs and open design questions are listed with
issue IDs (`#0001`, ...) in
[test/bugs_and_open_design_questions.md](test/bugs_and_open_design_questions.md);
the tests that cover them are reported as XFAIL and DESIGN. See
`test/run-tests.sh` for details. Logs are kept in `build/check`.

### Compilation Database (clangd)

Every `make` (and `make check`) writes `compile_commands.json` in the
top-level directory, a [compilation
database](https://clang.llvm.org/docs/JSONCompilationDatabase.html) for the
C++ sources of the compiler and the unit tests. It records the actual compiler
invocations of the last `make`, i.e., the selected C++ compiler (`CXX`), the
flags, the LLVM include directory and `-DLLVM_MAJOR_VERSION` of the selected
`llvm-config`, and the object files in `build.dir`. To make it describe
another LLVM version, run `make` with that configuration, e.g.

```bash
make llvm-config=llvm-config-23
```

The file is generated by `config/compile_commands.sh` and is not under version
control. To check that clangd uses it (`make check` does this automatically if
clangd is installed with the selected LLVM):

```bash
clangd --check=gen/function.cpp --check-locations=false --log=verbose 2>&1 |
    grep -E 'Loaded compilation database|Compile command from CDB|^E\['
```

clangd should report `Loaded compilation database from .../compile_commands.json`
and `Compile command from CDB is: ...` with the flags above, and no errors.

### Hints on Installing LLVM

ABC supports **LLVM 17 to 23** (including `llvm-config` and `clang`). The
complete test suite (`make check`) passes with each of these versions
(tested on macOS/arm64 with the Homebrew packages `llvm@17` to `llvm@23`).
The build configuration also accepts LLVM 14 to 16, but these versions are
not tested.

**macOS (recommended):**

```bash
brew install llvm@23
```

After installation, make sure the Homebrew LLVM tools are in your `PATH`:

```bash
export PATH="$(brew --prefix llvm@23)/bin:$PATH"
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

Windows is not supported natively.  
Please install **WSL (Windows Subsystem for Linux)** and use the Linux
instructions above.

## Declarations in C and ABC

For comparison, these declarations in C:
```c
    int *a[10];
    int (*b)[10];
```
are equivalent to these declarations in ABC:

<img width="261" height="46" alt="Screenshot 2026-09-05 at 05 14 11" src="https://github.com/user-attachments/assets/7d62422e-0d2b-4744-a34c-52b95ec97ad6" />


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

<img width="428" height="68" alt="Screenshot 2026-09-05 at 05 13 03" src="https://github.com/user-attachments/assets/b9372347-363e-47bd-b5c2-23ae229100eb" />


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

<img width="151" height="32" alt="Screenshot 2026-09-05 at 05 16 08" src="https://github.com/user-attachments/assets/a62e5b73-bb52-4563-b4da-5ecb1614a5a0" />


Hence the declaration of `someFunction` becomes

<img width="299" height="25" alt="Screenshot 2026-09-05 at 05 17 50" src="https://github.com/user-attachments/assets/cefdfa73-6f75-4b4f-a2d3-3c35f040e467" />


Let us consider some more exciting examples:

- With `g: -> fn(:int)` one declares a pointer to a function that has one
  parameter of type `int` and has no return type. Optionally you can use
  parameter names for readability which the compiler ignores. Hence `g: ->
  fn(value :int)` would be equivalent.
- With `h: -> fn(:int):int` or `h: -> fn(value :int):int` one declares a
  pointer to a function with one integer parameter and an integer return type.

Got the idea? Then you might already guess that

<img width="579" height="27" alt="Screenshot 2026-09-05 at 05 19 23" src="https://github.com/user-attachments/assets/21d7e29c-cbd6-4179-896e-98e58c04c7dd" />


declares a function pointer `foo` to a function with two integer parameters
which returns a pointer to a function that has one integer parameter and
returns a pointer to an integer. Now declare this in C without typedefs ;-)

# Examples

<img width="319" height="119" alt="Screenshot 2026-09-05 at 05 21 13" src="https://github.com/user-attachments/assets/1be9e28f-880a-4739-be34-bbcad7f62ca1" />

The worst part of C is the  C preprocessor (CPP). Hence it is greate that new C
like languages are avoiding the preprocessor. But because ABC is just "A Better
C, but still C" it does have a preprocessor. Students need to be prepared for
this ugly side of C. However, compared to CPP the preprocessor has limited
features. It can be used to include header files and you can define some simple
macros. 

Of course, the header file `stdio.hdr` does not contain the implementation of
`printf` but just a declaration for it. From the preprocessor the compiler gets
the following code:

<img width="369" height="121" alt="Screenshot 2026-09-05 at 05 22 41" src="https://github.com/user-attachments/assets/f56b4ab3-4c93-4e04-bf11-7ad8d335d156" />


Compared to using CPP no include guards are required when the ABC preprocessor
is used. Every file gets included only once (like using `@pragma once` with
CPPs that support this pragma). 

For teaching purposes (i.e. for showing the ugly side of C), consider this
example: 

<img width="246" height="115" alt="Screenshot 2026-09-05 at 05 24 27" src="https://github.com/user-attachments/assets/0cd86f67-0ace-41a7-b652-a791a9622dcb" />

Here the ABC compiler receives the following code from the preprocessor:

<img width="254" height="79" alt="Screenshot 2026-09-05 at 05 26 22" src="https://github.com/user-attachments/assets/c6e53797-5929-4c41-af20-36ed7f19ad36" />

Of course this triggers an error from the compiler. But it is hard to see from
the error message the actual problem:

<img width="628" height="53" alt="Screenshot 2026-09-05 at 05 29 24" src="https://github.com/user-attachments/assets/9f9073d5-bcc4-40e6-b3ea-054aafc7d4c7" />


Sure, the error message actually could show the code the compiler got from the
preprocessor. But using the preprocessor should not be attractive. If you want
to use symbols for literals use languages features, e.g. enum constants or
constant expressions. Intead of macro functions use inline functions. Don't use
a preprocessor.

## Raylib Examples

The [`raylib-examples`](raylib-examples/) directory contains a small collection
of ABC programs using [raylib](https://www.raylib.com/).

<img width="673" height="291" alt="Screenshot 2026-09-05 at 05 31 50" src="https://github.com/user-attachments/assets/80897c08-b127-475e-a369-ec3725577b04" />

<img width="822" height="492" alt="Screenshot 2026-09-05 at 05 33 54" src="https://github.com/user-attachments/assets/6b6bf7c1-57f8-4b48-9e3e-492e877e8a70" />



<img width="1105" height="583" alt="Screenshot 2026-09-05 at 08 00 29" src="https://github.com/user-attachments/assets/06e1aa10-1957-48a8-a53e-a0c5d9fc68e3" />



<img width="803" height="482" alt="Screenshot 2026-09-05 at 08 10 28" src="https://github.com/user-attachments/assets/49b02a10-8d41-43ee-8080-17e49c1030f4" />



If ABC is installed, the examples can simply be built with:

```sh
cd raylib-examples
make
```

The current examples are ports of selected C examples from the raylib example
collection. Translating these examples from C to ABC is mostly mechanical work
and can easily be done by hand or with a small script. Contributions adding
more examples are very welcome.

### ABI Support

Originally, ABC was developed for teaching purposes and did not need to
interoperate with external C libraries. Using raylib is a practical test case
for adding such interoperability and, in particular, for implementing
target-specific C ABI support in the compiler.

Currently, the raylib examples are supported only on x86-64. This is not
intended to be a fundamental limitation of ABC: the ABI layer is designed so
that support for additional target architectures can be added independently.

ARM support is planned next and should follow soon. Contributions implementing
or testing additional ABIs are very welcome.

## A Compiler Written in ABC

ABC was originally developed for my course
*Introduction to High Performance Computing*. As part of the course, students
write their own small compiler, **not-abc**, in ABC itself.

The [`not-abc`](not-abc/) directory contains the compiler developed by the
students during the semester and used for bootstrapping. If ABC is installed,
it can simply be built with:

```sh
cd not-abc
make
```

The separate [not-abc repository](https://github.com/michael-lehn/not-abc)
contains more information about the project, including the complete course
material, the compiler implementation, and the bootstrapping process.

# Language Description

Known deviations of the current compiler from this description and open
questions about the language are tracked in
[test/bugs_and_open_design_questions.md](test/bugs_and_open_design_questions.md).
References like *(open question #0024)* or *(compiler bug #0009)* refer to
entries in that file.

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
`}`         `(`         `)`         `[`         `]`         `?`
`+`         `++`        `+=`        `-`         `--`        `-=`
`->`        `*`         `*=`        `/`         `/=`        `%`
`%=`        `=`         `==`        `!`         `!=`        `>`
`>=`        `>>`        `>>=`       `<`         `<=`        `<<`
`<<=`       `&`         `&&`        `&=`        `|`         `||`
`|=`        `^`         `^=`        `#`

The punctuator `#` is not used by any grammar rule *(open question #0023)*.

The following keywords are reserved and cannot be used as identifiers:

`alignas`   `alignof`   `array`     `assert`    `break`
`case`      `const`     `continue`  `default`   `do`
`else`      `enum`      `extern`    `false`     `fn`
`for`       `global`    `goto`      `if`        `label`
`local`     `nullptr`   `of`        `readonly`  `return`
`sizeof`    `static`    `struct`    `switch`    `then`
`true`      `type`      `union`     `while`

The keywords `readonly` and `const` are synonyms. The keywords `alignas` and
`alignof` are reserved for operators that are not implemented yet *(compiler
bug #0020: the compiler does not reserve them)*.

Identifiers begin with a letter, i.e., `A` to `Z` and `a` to `z`, or an
underscore `_`, and are optionally followed by a sequence of more letters,
underscores, or decimal digits `0` to `9`. Some identifiers are predefined.

The following predefined identifiers are used as named
[types](#types) (essentially keywords):

`void`              `bool`              `char`              `u8`
`u16`               `u32`               `u64`               `i8`
`i16`               `i32`               `i64`               `int`
`long`              `long_long`         `unsigned`          `unsigned_long`
`unsigned_long_long` `size_t`           `ptrdiff_t`         `float`
`double`

`char` is an unsigned 8-bit integer type and `bool` is an integer type with
the values `false` (0) and `true` (1). *Compiler bug #0021: `long_long`,
`unsigned_long` and `unsigned_long_long` are currently not defined; compiler
bug #0009: `long` currently has 32 bits on all targets. Open question #0029:
`size_t` and `ptrdiff_t` are currently signed.*

The keyword `nullptr` denotes the null pointer; `true` and `false` are the
values of type `bool`.

Literals can be decimal literals, octal literals, hexadecimal literals, string
literals, character literals, and (decimal) floating point literals.
- Decimal literals begin with a digit `1` to `9` and optionally have more
  digits from `0` to `9`. Decimal constants are unsigned and can be of
  arbitrary size. *(Open question #0027: the compiler currently gives an
  integer literal without type suffix the type `int` if the value fits,
  otherwise `i64`.)*
- Octal literals begin with a digit `0` and optionally have more digits from
  `0` to `7`.
- Hexadecimal literals begin with a prefix `0x` (or `0X`) and one or more
  digits from `0` to `9`, `a` to `f`, or `A` to `F`. *(Compiler bug #0005:
  hexadecimal literals currently have type `i32` and larger values are
  truncated.)*
- An integer literal can be directly followed by the name of an integer type,
  which then is the type of the literal, e.g., `255u8` or `1i64`.
- String literals are delimited by `"`. Backslashes, i.e., `\`, are escape
  characters, removing the special meaning of the following character or
  allowing the insertion of special characters into a string. The escape
  sequences are those of C: `\'`, `\"`, `\?`, `\\`, `\a`, `\b`, `\f`, `\n`,
  `\r`, `\t`, `\v`, octal escapes with one to three digits, and hexadecimal
  escapes `\x...` *(compiler bug #0012: octal escapes in string literals are
  currently broken)*. Adjacent string literals are concatenated.
- Character literals are delimited by `'` and consist of a single character
  (which can be an escaped character).
- Currently only decimal (but not hexadecimal) floating point literals are
  supported (see
  [floating_literal](https://en.cppreference.com/w/cpp/language/floating_literal)).

## Preprocessor Directives

A directive begins with `@` and ends at the end of the line:

- `@ <file>` includes a file that is searched in the directories given with
  `-I` and in the installed ABC include directory. `@ "file"` includes the
  file with the given path. Each file is included at most once, so include
  guards are not needed.
- `@define NAME tokens...` defines a macro: every following occurrence of the
  identifier `NAME` is replaced by the tokens. The replacement can be empty.
- `@ifdef NAME` ... `@endif` keeps the enclosed tokens only if `NAME` is a
  defined macro. These directives cannot be nested.

## Expressions

The syntax for expressions is very similar to expressions in C, with the
following exceptions:
- The operators `alignas` and `alignof` are not implemented yet.
- For a conditional expression, the ternary Operator from C can be used, e.g.,
  `x = a > b ? y : z`, or alternatively, the more verbose notation `x = a > b
  then y else z`. At least currently, "?" and "then", and ":" and "else" are
  interchangeable. That means `x = a > b ? y else z` and `x = a > b then y : z`
  are also alternatives. *(Open question #0026: the compiler currently only
  accepts "?" together with ":" and "then" together with "else".)*
- Type casts: the compiler implements the C-style cast `(T)expr` and the
  compound literal `T{expr}`, e.g., `x = (int)y` or `x = int{y}`, for
  converting the expression `y` to type `int`. Previous versions of this
  description specified the C++ style notation `x = int(y)` instead of the C
  syntax; `int(y)` is not implemented *(open question #0024)*.
- Pointers can be dereferenced like in C with the prefix operator `*`.
  Additionally, the arrow operator `->` can be used; i.e., the use of the
  operator `->` is not restricted to "struct pointers". Hence, if `x` is a
  pointer to `int`, the expressions `*x = 42` and `x-> = 42` are equivalent,
  and in both cases, `42` is assigned to the integer at the end of pointer `x`.
- A function name does not convert implicitly to a function pointer, and a
  function pointer has to be dereferenced to be called: with
  `p: -> fn(:int): int`, write `p = &f` and `p->(1)` or `(*p)(1)`.
- `assert(expression)` aborts the program with a diagnostic if the expression
  is false.

The EBNF grammar for expressions is:

```ebnf
          expression-list = assignment-expression { "," assignment-expression }
    assignment-expression = conditional-expression
                                [ assignment-operator assignment-expression ]
      assignment-operator = "=" | "+=" | "-=" | "*=" | "/=" | "%="
                          | "&=" | "|=" | "^=" | "<<=" | ">>="
   conditional-expression = logical-or-expression
                                [ ("?" | "then") assignment-expression
                                  (":" | "else") conditional-expression ]
    logical-or-expression = logical-and-expression { "||" logical-and-expression }
   logical-and-expression = bitwise-or-expression { "&&" bitwise-or-expression }
    bitwise-or-expression = bitwise-xor-expression { "|" bitwise-xor-expression }
   bitwise-xor-expression = bitwise-and-expression { "^" bitwise-and-expression }
   bitwise-and-expression = equality-expression { "&" equality-expression }
      equality-expression = relational-expression { ("==" | "!=") relational-expression }
    relational-expression = shift-expression { ("<" | "<=" | ">" | ">=") shift-expression }
         shift-expression = additive-expression { ("<<" | ">>") additive-expression }
      additive-expression = multiplicative-expression { ("+" | "-") multiplicative-expression }
multiplicative-expression = unary-prefix-expression { ("*" | "/" | "%") unary-prefix-expression }
  unary-prefix-expression = ("-" | "!" | "++" | "--" | "*" | "&") unary-prefix-expression
                          | "(" type ")" unary-prefix-expression
                          | postfix-expression
       postfix-expression = primary-expression
                          | postfix-expression "." identifier
                          | postfix-expression "->" [ identifier ]
                          | postfix-expression "[" expression-list "]"
                          | postfix-expression "(" [ argument-list ] ")"
                          | postfix-expression "++"
                          | postfix-expression "--"
            argument-list = assignment-expression { "," assignment-expression }
       primary-expression = identifier
                          | "(" expression-list ")"
                          | compound-literal
                          | "sizeof" "(" (type | expression-list) ")"
                          | "assert" "(" expression-list ")"
                          | "nullptr" | "true" | "false"
                          | integer-literal
                          | floating-point-literal
                          | character-literal
                          | string-literal { string-literal }
         compound-literal = type-name compound-expression
                          | "(" type ")" compound-expression
```

`integer-literal` is a decimal, octal or hexadecimal literal with optional type
suffix, and `type-name` is an identifier that names a type.

```ebnf
            compound-expression = string-literal { string-literal }
                                | "{" compound-expression-initializer
                                      { "," compound-expression-initializer } [ "," ] "}"
compound-expression-initializer = [ designator "=" ] ( compound-expression | assignment-expression )
                     designator = "." identifier
                                | "[" assignment-expression "]"
```

A compound expression is the value of an array or struct (or, with a single
element, of a scalar). A string literal is a compound expression for an array
of characters. Elements that are not initialized are zero.

For convenience, the precedence and associativity are summarized in the following table:

| Precedence    |   Associativity   |   Operators                           |   Meaning     |
|---------------|-------------------|---------------------------------------|---------------|
| 16 (highest) |  left | Identifier <br> Literal <br> `++` (post-increment) <br> `--` (post-decrement) <br> `f()` (function call) <br> `[i]` (index operator) <br> `->` (indirect member access or dereference operator) <br> `s.member` (direct member access) <br> `T{...}` (compound literal) | Primary and  Unary postfix expression |
| 15 | right | `*` (dereference operator) <br> `&` (address operator) <br> `-` (unary minus) <br>  `+` (unary plus, *open question #0025*) <br>  `!` (logical not) <br> `++` (pre-increment) <br> `--` (pre-decrement) <br> `sizeof` <br> `(type)` (cast) <br> type(expression) (*open question #0024*) | Unary prefix expression |
| 13 | left | `*` (multiply) <br>  `/` (divide) <br> `%` (modulo) | Multiplicative expression |
| 12 | left | `+` (add) <br> `-` (subtract) | Additive expression |
| 11 | left | `<<` (left shift) <br> `>>` (right shift) | Shift expression |
| 10 | left | `<`  (less) <br> `>`  (greater) <br> `<=` (less equal) <br> `>=` (greater equal) | Relational expression |
| 9  | left | `==` (equal) <br> `!=` (not equal) | Equality and inequality expression |
| 8  | left | `&` | Bitwise and |
| 7  | left | `^` | Bitwise exclusive or |
| 6  | left | `\|` | Bitwise or |
| 5  | left | `&&` | Logical and |
| 4  | left | `\|\|` | Logical or |
| 3  | right | `?` in conjunction with `:` or `then` in conjunction with `else` | Conditional expression |
| 2  | right | `=` <br> `+=` <br> `-=` <br> `*=` <br> `/=` <br> `%=` <br> `&=` <br> `\|=` <br> `^=` <br> `<<=` <br> `>>=` | Assignment |
| 1  | left | `,` | Expression list |

Unary plus is listed in the table but not in the grammar *(open question
#0025)*. Mixing signed and unsigned operands follows rules that are not
specified yet *(open question #0028)*.

## Types

```ebnf
                 type = [ "readonly" | "const" ] unqualified-type
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

The dimension of an array is a constant integer expression. In the type of a
variable definition with initializer, or of an extern variable declaration,
the first dimension can be empty (`array[] of int`); the dimension is then
determined by the initializer (or the definition).

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
           function-parameter-list = [ parameter { "," parameter } [ "," "..." ] ]
                         parameter = [identifier] ":" type
                     function-body = compound-statement
```

```ebnf
         extern-declaration = "extern" ( function-declaration | extern-variable-declaration ) ";"
       function-declaration = function-header
extern-variable-declaration = identifier-list ":" type { "," identifier-list ":" type }
            identifier-list = identifier { "," identifier }
```

#### Global Variable Declarations and Definitions

```ebnf
global-variable-definition = "global" variable-definition-list ";"
  variable-definition-list = variable-definition { "," variable-definition }
       variable-definition = identifier-list ":" type [ "=" initializer-expression ]
                           | identifier-list ":" [ "readonly" | "const" ] "=" initializer-expression
    initializer-expression = compound-expression
                           | assignment-expression
```

In the second form, usually written `x := expression`, the type of the variable
is the type of the initializer. If the identifier list has more than one
identifier, the initializer is a compound expression with one element per
variable, e.g., `local a, b: int = {1, 2};` or `local x, y := {1, 2.5};`.

#### Type Aliases

```ebnf
type-declaration = "type" identifier ":" type ";"
```

#### Structured Type Declaration

```ebnf
       struct-declaration = "struct" identifier (";" | struct-member-declaration )
struct-member-declaration = "{" { union-section | struct-member-list } "}" ";"
            union-section = "union" "{" struct-member-list { struct-member-list } "}" ";"
       struct-member-list = identifier { "," identifier } ":" ( type ";" | struct-declaration )
```

The members of a union section share their storage.

#### Enumeration Type Declaration and Enumeration Constants

```ebnf
  enum-declaration = "enum" identifier [ ":" integer-type ]
                         ( ";" | "{" enum-constant-list [ "," ] "}" ";" )
enum-constant-list = identifier [ "=" assignment-expression] { "," identifier [ "=" assignment-expression] }
```

The default integer type is `int`. `enum E;` (or `enum E : u8;`) declares an
enumeration type that is defined later *(compiler bug #0032: a definition
with a different integer type is not rejected)*.

### Statements

#### Compound Statements

```ebnf
            compound-statement = "{" { statement | declaration } "}"
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
                     [ "else" ( if-statement | compound-statement ) ]
```

##### Switch statements

```ebnf
        switch-statement = "switch" "(" expression-list ")" "{" { switch-case-or-statement } "}"
switch-case-or-statement = "case" expression-list ":"
                         | "default" ":"
                         | statement
```

Like in C, execution continues with the statements of the following case
unless a `break` statement leaves the switch. *Open question #0031: the
meaning of a case label with more than one expression, e.g. `case 3, 4:`.*

##### Loops

###### While Loops

```ebnf
while-statement = "while" "(" expression-list ")" compound-statement
```

###### Do-While Loops

```ebnf
do-while-statement = "do" compound-statement "while" "(" expression-list ")" ";"
```

###### For Loops

```ebnf
for-statement = "for" "(" for-init [expression-list] ";" [expression-list] ")"
                          compound-statement
     for-init = local-variable-definition
              | [expression-list] ";"
```

###### Break and continue

```ebnf
break-statement = "break" ";"
continue-statement = "continue" ";"
```

###### Return

```ebnf
return-statement = "return" [ expression-list ] ";"
```

###### Goto and labels

```ebnf
  goto-statement = "goto" identifier ";"
label-definition = "label" identifier ":"
```


## License

Copyright (C) 2023-2026 Michael C. Lehn, Ulm University

This project is licensed under the GNU General Public License
version 3 or later. See [LICENSE](LICENSE) for details.
