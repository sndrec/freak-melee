# Stage collision line operations

Line inputs must be integral values that fit a native signed 32-bit index.
Indices refer to the current stage collision geometry. Check a retained line
again before using it in a later callback.

`stage.line_exists(line)` returns whether the current line is enabled and
visible. Missing geometry, negative indices and indices beyond the current
line count return `false`. The guard runs before the native lookup, avoiding
its invalid-index diagnostic hang.

`stage.line_normal(line, x, y, z)` calls the native `mpLineGetNormal` operation
and writes three distinct `float` destinations. It requires an existing enabled,
visible line. The native normal follows the current endpoint positions; Z is
zero. State, local and supported writable engine property destinations use
the interpreter's standard output publication rules.

`item.bind_stage_line(line)` calls the native `mpColl_80043558` operation on
the Article's actual collision record. A floor line dispatches its joint's
floor contact callback with native mode 2; a ceiling line dispatches the ceiling
callback with mode 0. A wall line or a joint without the selected callback
causes no callback. The operation does not itself write a stored line index.
It returns no value.

The collision record must name the actual Item. Category 1 is a Fighter contact
and is rejected for an Item. When a selected callback exists, its collision
amount must fit the native callback's `int` parameter; the kernel converts the
record's float amount before entering the callback. Ordinary native callback
side effects, including hiding a contacted line, remain intact.

`item.read_stage_line_speed(line)` calls native `mpGetSpeed` using the Article's
position and velocity fields. It replaces velocity XY with the stage's remapped
line motion and sets velocity Z to zero. The position stays unchanged. It
returns the native operation's `bool` result. Both Item line operations require
an existing enabled, visible line before entering the kernel.

All four operations guard the source's physical identity, scene, generation,
owned program and storage before result publication. Geometry pointers and
structural indices are pinned across native contact callbacks. Completed native
callback side effects are not rolled back if publication fails. These guards
do not claim containment of arbitrary destructive native callbacks.

The [connected comparison fixture](../../tests/native/script_item_sweep/README.md)
executes these unchanged engine kernels over controlled line geometry and
contact callbacks, alongside the Item segment sweep.
