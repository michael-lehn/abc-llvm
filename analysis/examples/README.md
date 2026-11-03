# Examples for static analysis

Each program says in its first line which options generate a warning, and marks
the line that gets it with the expected message. `Warning options: none` means no
warning with any option. `test/driver/analysis_examples.sh` (test T0289) checks
that these comments are correct.

```bash
abc -Wall -c uninitialized_always.abc
```

## Options

| Program | Shows | `-Wuninitialized` | `-Wnull-dereference` | `-Wnull-dereference=strict` | `-Wall` |
|---------|-------|---|---|---|---|
| `uninitialized_always.abc` | no path assigns a value | warning |  |  | warning |
| `uninitialized_some_paths.abc` | only some paths assign a value | warning |  |  | warning |
| `uninitialized_short_circuit.abc` | control flow of `&&` | warning |  |  | warning |
| `uninitialized_goto.abc` | `goto` over an initializer | warning |  |  | warning |
| `uninitialized_not_detected.abc` | undefined behavior no option finds, address taken |  |  |  |  |
| `null_malloc_unchecked.abc` | `malloc` can return `nullptr` |  | warning | warning | warning |
| `null_malloc_tested.abc` | the ways to test a pointer |  |  |  |  |
| `null_known_null.abc` | `is null` instead of `may be null` |  | warning | warning | warning |
| `null_after_loop.abc` | what a loop condition tells |  | warning | warning | warning |
| `null_use_after_free.abc` | dereference after `free` |  | warning | warning | warning |
| `null_double_free.abc` | `free` twice |  | warning | warning | warning |
| `null_test_in_variable.abc` | false warning of the simple analysis only |  | warning |  |  |
| `null_flag_on_path.abc` | false warning of both analyses |  | warning | warning | warning |
| `null_test_after_dereference.abc` | a test of a parameter that comes too late |  |  | warning | warning |
| `member_unknown.abc` | an unknown member may be null |  |  | warning | warning |
| `member_tested.abc` | testing a member |  |  |  |  |
| `member_forgotten.abc` | a call forgets what was tested |  |  | warning | warning |
| `member_assigned.abc` | state from an assignment to a member |  |  | warning | warning |
| `member_guaranteed.abc` | false warning, the price of being strict |  |  | warning | warning |

The false warnings are there on purpose. An analysis can not avoid both, missing
bugs and warning about correct programs, so it has to choose.
