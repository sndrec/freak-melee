# Smash charge state

`smash` exposes the engine's per-fighter charge and shake state. Each fighter
instance has its own values; these are not special variables owned by Ness or
any other stock character. Read and write them from fighter callbacks or helpers.
Changing these fields does not enter an action, bind callbacks or run a stock
fighter's charge handler.

## Properties

| Property | Type | Access | Meaning |
| --- | --- | --- | --- |
| `smash.charge_mode` | Int | Read/write | Shared charge state; use a `Smash` constant below. |
| `smash.charge_frames` | Float | Read/write | Accumulated charge frames. |
| `smash.charge_limit` | Float | Read/write | Native hold-frame limit. |
| `smash.saved_animation_rate` | Float | Read/write | Animation rate saved for charge/release handling. |
| `smash.charge_color` | Int | Read/write | Native charge color-animation ID. Setting it does not start a color animation. |
| `smash.shake_index` | Int | Read/write | Shake-table index byte, 0 through 255. |
| `smash.shake_period` | Int | Read/write | Native shake period/count byte, 0 through 255. |
| `smash.shake_table_count` | Int | Read only | Count from the shared native smash shake table. |

## Charge modes

| Constant | Value |
| --- | --- |
| `Smash.none` | 0 |
| `Smash.precharge` | 1 |
| `Smash.charging` | 2 |
| `Smash.release` | 3 |
| `Smash.manual_charge` | 4 |

The first four are the native `SmashState` enum. Mode 4 is used by stock Ness's
Yo-yo charge code even though it has no enum name there. It selects the manual
charge policy; the script owns its move decisions and elapsed-charge updates.

```text
func begin_manual_charge():
    smash.charge_mode = Smash.manual_charge
    smash.saved_animation_rate = animation.rate
    smash.charge_frames = 0.0
    smash.charge_limit = 60.0
    smash.shake_index = 0
    smash.shake_period = smash.shake_table_count
```

These properties operate on the exact live physical Fighter allocation and
scene generation. A stale/retired source, wrong type or invalid mode/byte range
produces a diagnostic without changing that field. The shake count requires the
shared table to be initialized. Float values retain their native representation;
these setters do not implement a new charge formula or silently clamp timing.

See [motion callbacks](motion_callbacks.md), [callback slots](callback_slots.md)
and [movement](movement.md) for the surrounding move lifecycle. The
[common-state checkpoint](../native_script_ness_common_state_checkpoint.md)
records interpreter/native comparisons and remaining Ness preparation gaps.
