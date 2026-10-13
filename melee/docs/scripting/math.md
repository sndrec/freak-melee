# Math and vectors

Use these operations in fighter functions or article callbacks. They operate
on numbers and directions; no operation depends on a fighter or move name.

| Operation | Description |
| --- | --- |
| `min(a, b)` / `max(a, b)` | Return the smaller/larger float; useful for clamping owned charge and speed values. |
| [`atan2(y, x)` / `atan2_float(y, x)`](#atan2-and-atan2_float) | Return a signed radian angle, with double or float math respectively. |
| [`wrap_signed_angle(value)`](#wrap_signed_angle) | Normalize a radian angle to the signed interval. |
| [`vectors.normalize(x, y, z)`](#vectorsnormalize) | Normalize three writable float values in place. |
| [`vectors.rotate(x, y, z, axis, angle, out_x, out_y, out_z)`](#vectorsrotatex-y-z-axis-angle-out_x-out_y-out_z) | Rotate a copied vector about X, Y or Z with three Float outputs. |
| [`vectors.angle(ax, ay, az, bx, by, bz)`](#vectorsangle) | Read the unsigned angle between two 3D vectors. |
| [`vectors.angle_xy(ax, ay, bx, by)`](#xy-vector-operations) | Read the unsigned angle between two XY vectors. |
| [`vectors.cross_normalized_x/y/z(ax, ay, az, bx, by, bz)`](#normalized-cross-product) | Read a component of the normalized cross product in fighter or article code. |
| `random.float()` / `random.int(upper_bound)` | Draw from the engine RNG shared by simulation; the integer result is in `[0, upper_bound)`. |

`random.float()` is available in both fighter and article expressions. Fighter
scripts also accept `random()` for the same draw. Calling it advances the shared
RNG; store a result in a local variable when several decisions should use one
draw. CPU spacing updates receive their already-drawn factor as `event.value`.

## `double(value)`

Explicitly promotes a numeric value to binary64. Arithmetic involving that
result uses double precision; assignment to a `float` local or field rounds
the result back to binary32. Decimal literals begin as binary64; use
`float(value)` first when promoting a specifically float-rounded constant.
For example, the native
Hookshot spool performs its interpolation in double precision and then stores
a float: `double(float(0.65)) * (double(1) - double(fraction)) + double(float(0.35))`.
This uses the existing VM numeric conversion and introduces no engine state.

## atan2 and atan2_float

In fighter and model-effect expressions, `atan2(y, x)` uses the engine's
double-precision `atan2` operation. `atan2_float(y, x)` uses `atan2f`, with
float arguments and a float result. Article expressions retain their existing
float `atan2(y, x)` operation.
Both take Y first, X second and return radians. Use the float form when
translating a stock operation that explicitly calls `atan2f`; rounding a double
result afterward is not guaranteed to produce the same float in every case.

[Animated model effects](model_effects.md) support both forms. For example,
`-atan2_float(creator.floor.normal.x, creator.floor.normal.y)` aligns the
model's Z rotation with the floor using the stock swirl convention. The
normal's component order in that example is intentional.

## wrap_signed_angle

`wrap_signed_angle(value: float) -> float`

Available in fighter and article expressions. Returns an angle between
`-pi` and `pi` radians. It uses the shared item convention's double constants
and rounds on each float assignment, preserving ordinary stock calculations.
Nonfinite input returns zero; very large finite input uses a bounded reduction.

```text
item.state.angle = wrap_signed_angle(item.state.angle + pi)
```

This differs from the existing article `wrap_angle()`, whose interval is
`[0, tau]`. See [launch initialization](article.md#launch-initialization) for
world-space launch angles and [article variables](article.md#local-variables)
for storing a move's own trajectory representation.

## vectors.normalize

`vectors.normalize(x: float inout, y: float inout, z: float inout)`

Available in fighter and article statements. Arguments must be writable Float
destinations: local variables, fighter/article state or writable Float properties.
Literals, computed expressions and Int destinations are rejected. Reads all
three inputs before publishing the normalized values. Uses the shared engine's
`lbVector_Normalize`, including its zero-length, rounding and nonfinite behavior;
it does not add a replacement math policy. Uses one stack vector and allocates
nothing. A retired caller cannot publish the result.

```text
item.state.direction_x = velocity.x
item.state.direction_y = velocity.y
item.state.direction_z = velocity.z
vectors.normalize(item.state.direction_x, item.state.direction_y, item.state.direction_z)
```

## vectors.angle

`vectors.angle(ax: float, ay: float, az: float, bx: float, by: float, bz: float) -> float`

Available in fighter and article expressions. Returns the shared engine's angle between
two vectors in radians, using its dot-product and magnitude handling. The
inputs need not be normalized. The result is unsigned; scripts decide the
turn direction from their own orientation or a signed angular difference.

```text
item.state.turn = vectors.angle(1, 0, 0, velocity.x, velocity.y, velocity.z)
```

[Collision normals](item.md#stage-contact-and-collision-box) can be accumulated
and normalized to steer an article around surfaces. [Bowser's translation](../bowser_script_translation.md)
uses those general operations; its steering decisions live in the script.

Decimal literals in interpreted article and fighter expressions begin as
binary64. Float locals, parameters, fields and float-only call arguments round
to binary32; `float(value)` requests that conversion inside an expression.
`pi` and `tau` deliberately retain the shared engine's double
constants. Preserve explicit intermediate assignments when translating stock
code whose rounding occurs before the next operation.

## XY vector operations

`vectors.normalize_xy(x, y, z)` takes three writable Float destinations in
fighter or article statements, just like `vectors.normalize`. It normalizes only X and Y and
leaves Z unchanged; a zero-length XY vector remains unchanged.

`vectors.angle_xy(ax, ay, bx, by)` accepts four expressions and returns an
unsigned angle in radians. Both input Z components are zero. This is the same
four-argument contract in fighter and article scripts; six arguments are rejected.
Zero XY magnitude returns zero. The XY angle routine tests a nonzero length
product, while the 3D angle routine tests a product greater than `1e-10f`.
Those native distinctions, rounding and nonfinite results are preserved.

## vectors.rotate_z

`vectors.rotate_z(x, y, z, angle)`

All four arguments must be distinct declared `item.state` float variables.
The angle variable is read in radians; X and Y are rotated in place and Z
is preserved. Melee's polynomial rotation routine differs slightly from
ordinary `sin`/`cos` rotation and does not renormalize the result. Store an
expression in the angle variable before calling it.

```text
item.state.turn_angle = item.state.turn * -facing
vectors.rotate_z(item.state.direction_x, item.state.direction_y, item.state.direction_z, item.state.turn_angle)
```

See [target queries](targets.md) for finding a direction to steer toward.

## Bitwise expressions

Fighter expressions support `&`, `|`, `^` and `~` on integer operands, with
ordinary C precedence. For example, combine named motion-transition flags
with `|` when calling [`state.enter_at_speed`](fighter.md). These operators do not
convert floating-point values into bit patterns; use integer operands.

## Unsigned integer arithmetic

`uint(value: int)` is available in fighter and article expressions. Integer
conversion and unsigned arithmetic use 32 bits. Use it for wrapping counters
and serial-number differences; ordinary declared `state int` storage retains
the resulting 32-bit pattern.

```text
state.serial = uint(state.serial) + 1
```

An article can compare generations across a counter wrap:

```text
if uint(owner.state.serial) - uint(item.state.serial) >= 2:
    item.signal("expire", 0)
```

Keep these operands integer-valued. This conversion does not clamp a float
or provide a validated float-to-integer conversion.

## Normalized cross product

`vectors.cross_normalized_x(ax, ay, az, bx, by, bz) -> float`

The `_y` and `_z` variants return the other components. All six inputs are
expressions describing two world-space vectors. The engine computes `a cross b`,
normalizes that result and returns the requested component. Operand order
matters: swapping the vectors reverses the result. Each call uses stack vectors
and allocates nothing; it neither changes the inputs nor advances RNG.

```text
let float turn_axis_z = vectors.cross_normalized_z(1, 0, 0, velocity.x, velocity.y, velocity.z)
```

Available in fighter functions and article callbacks. The shared Melee vector
routine owns zero-length and near-zero handling; scripts do not substitute a
move-specific fallback direction. For the magnitude or signed angle of a turn,
use the [angle operations](#vectorsangle) and your own orientation state.

## `vectors.rotate(x, y, z, axis, angle, out_x, out_y, out_z)`

Rotate a vector with Melee's `lbVector_Rotate`. Available in fighter functions
and article callbacks. `axis` is an Int: `Axis.x` (1), `Axis.y` (2), or
`Axis.z` (4). `angle` is in radians. The three outputs are writable Float
locals; inputs are copied before results are published. No actor velocity or
position is used as temporary storage.

```text
let float x = 0
let float y = 0
let float z = 0
vectors.rotate(1, 0, 0, Axis.z, 0.5, x, y, z)
```

An unsupported axis fails without changing the outputs. The native function
has no defined result for other axis codes, so scripts cannot pass them through.
Rotation preserves Melee's native trigonometric implementation and floating
operation order. It allocates no memory or retained pointers.

To rotate the fighter's actual self velocity about a supplied unit axis, use
[`physics.rotate_velocity_unit_axis`](movement.md#vector-velocity-transforms).
