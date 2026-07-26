# not-abc (ABC implementation)

This directory contains a compiler for the tiny programming language
**not-abc** written in **ABC**.

The compiler was developed as part of the undergraduate course *Introduction to
High Performance Computing*. It is considerably easier to read than the
self-hosting version because it still makes use of language features such as
structures, enumerations, and member access.

## Building

This directory assumes that the ABC compiler has already been built and
installed.

Simply run

```sh
make
```

The main compiler executable is

```text
not-abc-written-in-abc
```

It reads `not-abc` source code from **stdin** and writes the generated code to
**stdout**.

## Compiler Backends

Several compiler variants are built.

### LLVM backend

```text
not-abc-written-in-abc_llvm
```

Generates LLVM IR. The generated code can be compiled into a native executable
using the LLVM toolchain, for example with Clang.

Example:

```sh
./not-abc-written-in-abc_llvm < examples/hello.abc > hello.ll
clang hello.ll -o hello
```

### ULM backend

```text
not-abc-written-in-abc_simple
```

Generates assembly for the ULM architecture specified by `simple.isa`.

If the **ulm-generator** tools are installed, `make` also builds

- the assembler,
- the virtual machine,
- and the TUI debugger

for this architecture.

## Examples

The `examples/` directory contains several example programs that can be
compiled with either backend.

## Compiler Wrappers

For convenience, the installation also provides wrapper scripts in

```text
~/.local/bin
```

These hide the individual compilation steps.

Instead of

```sh
./not-abc-written-in-abc_llvm < examples/foo.abc > foo.ll
clang foo.ll -o foo
```

you can simply write

```sh
not-abc_llvm examples/foo.abc
```

which produces

```text
a.out
```

or

```sh
not-abc_llvm examples/foo.abc -o foo
```

which produces

```text
foo
```

The wrappers are installed automatically. Sorry, they do not ask for permission
first—I tend to prefer asking for forgiveness afterwards. 🙂
