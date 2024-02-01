# abc-llvm
A Bloody Compiler (with LLVM backend)

The ABC language is essentially C with Pascal-like syntax for declarations.

For comparison, these declarations in C:
```c
    int *a[10];
    int (*b)[10];
```
are equivalent to these declarations in ABC:

```abc
    a: array[10] of -> int,
    b: -> array[10] of int,
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
    c: array[10] of -> const int,
    d: array[10] of const -> int,
    e: array[10] of const -> const int;
```
Here, we have declared

- `c` as an array of 10 elements, where each element is a pointer to a constant integer
- `d` as an array of 10 elements, where each element is a constant pointer to an integer
- `e` as an array of 10 elements, where each element is a constant pointer to a constant integer
