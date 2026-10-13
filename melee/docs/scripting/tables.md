# Constant tables

A table is an immutable, typed list for lookup data: recoil curves, probabilities,
state choices or other values selected by a computed index. Its contents are
evaluated when the script loads and shared by actors running that script. A table
does not occupy [fighter state](language.md#fighter-state) or get copied into each
actor's rollback snapshot.

```text
table float recoil = [0, -0.51, -1.02, -1.53]
table int choices = [0, 9, 9, 3]
table bool allowed = [false, true, true]

func apply_recoil(int step):
    let float displacement = table.recoil[step]
    velocity.z = displacement
```

Declare tables at the script root or directly inside an `article`, `item` or
`model_effect`. Use `table.NAME[index]` to read one element. An entity's local table
takes precedence over a root table with the same name. `program.table.NAME[index]`
always selects the executing package's root table, just as
[`program.attrs`](language.md#fighter-attributes) selects its root parameters.
Tables in another entity's scope are not accessible.

## Element types and defaults

Supported element types are **bool**, **int**, **uint** and **float**. Each element
uses the same constant-expression rules and scalar conversions as a parameter
default. Arithmetic and literal engine constants are allowed; runtime calls,
fighter state, parameter references and other table reads are not. Float elements
must be finite. Int and Uint conversion retains the language's 32-bit wrapping
semantics; use values within the intended range unless that wrapping is deliberate.

```text
table float offsets = [-0.5, (2 + 3) / 2.0]
table uint masks = [0, 4294967295]
```

Each table needs at least one element. Duplicate names in the same scope, empty
elements and trailing commas are errors. Current fixed limits are **64 tables**
and **2,048 elements in total** per script. Each element takes four immutable
bytes; Bool elements are normalized to zero or one. There is no allocation during
lookup.

## Indexing

Indices must be integral and satisfy `0 <= index < element_count`. A negative or
out-of-range index faults the script invocation and leaves the read output
unpublished. There is no implicit clamping or wraparound of indices.

Tables are one-dimensional and read-only. An assignment such as
`table.recoil[0] = 2`, a float index or a second index is rejected when preparing
the script. To edit persistent runtime values, use an ordinary
[state array](language.md#fighter-state). To change a lookup table's contents,
edit its declaration and reload the draft.

See the [constant-table checkpoint](../native_script_table_checkpoint.md) for
connected actor/VM tests, preparation results and remaining fighter work.
