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
f: -> fn()
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
