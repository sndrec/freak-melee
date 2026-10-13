# Fighter-owned parameter checkpoint

The authoring contract is documented in the
[scripting reference](scripting/language.md#fighter-attributes).
Projects own `fighter_parameters.fmp`; the original DAT remains unchanged.
The generated behavior module reads an indexed, fixed-capacity table bound
when its package loads. There is no per-tick parsing or allocation.

Names, types, and order define the schema. Editing values only changes the
package; renaming, adding, removing, reordering, or changing the type of a
field requires rebuilding behavior. A mismatched schema is rejected before
gameplay. Parameters also belong to scripted articles/items after reflection
or a holder change. Mutable resources belong in rollback-tracked `state`.

## Stock import

The generic importer consumes `attribute float/int/bool/byte Name OFFSET`
declarations and copies each original big-endian DAT value into an owned
parameter. Float text preserves the original binary32 value. It removes the
declarations from the resulting script, leaving move logic intact. Article
defaults become qualified owned names through the same import path. Opening
an existing Workshop draft performs this
conversion in the working draft; Save and Build commit it. The explicit
`import-parameters` command offers the same operation for source workflows.
Unsigned byte imports become ordinary editable integers; boolean imports
require a byte of zero or one. The [ribbon reference](scripting/trail.md) connects
packed stock colors to a generic authored rendering description.

Mario and Dr. Mario's full templates now name their primitive move values,
so both use this path. Luigi already declares named imports and uses it
without rewriting move logic. Captain Falcon and Ganondorf also use named
imports; their fresh parameter-backed comparison checkpoint is documented in
the [family translation ledger](captain_ganon_script_translation.md).
The original attribute block is still retained
for common engine readers, imported article data, and structured engine data
such as reflector descriptions. Other translation drafts and older examples
still compile against imported DAT values until converted; this checkpoint
does not claim that every engine/DAT dependency has been eliminated.

## Charge lifecycle

The full Mario and Dr. Mario scripts declare `state.tornado_charge` and
`state.cape_boost_used`. Death and the corresponding special collision
callbacks reset them. `on landing_action` resets both at the common landing
action boundary used by stock. It is distinct from the earlier `on landing`
air-to-ground event, which can keep a special active. This new callback is
fighter-agnostic. The old isolated Cape/Tornado examples have been retired;
Cape lifecycle coverage lives in a test fixture. The compiler charge shortcuts
and hard-coded numeric Mario attribute aliases have been removed. New scripts
must use ordinary fighter state and declare their named values.

## Validation on 2026-09-28

The Release client, content tool, and complete configured build succeed.
Seventeen scripting/content checks passed, including execution of a loaded
module with float/int/bool parameters, schema rejection, source/package
round trips, and bit-for-bit Luigi stock-value import. The DVD transfer test
also passed after removing its obsolete runtime symbol overrides and
initializing the real audio heap used for DevCom transfer buffers.

Ten rendered stock-versus-clone comparisons each completed 520 controller
samples with matching observable fighter, action, item, damage, and RNG
traces. The cloned modules used converted project-owned parameters:

| Fighter | Recorded paths |
| --- | --- |
| Mario | Air Cape, air Tornado, repeated Tornado |
| Dr. Mario | Air Cape, repeated Tornado, death/respawn, Cape reflecting a pill |
| Luigi | Held Green Missile release, seeded misfire, air Cyclone contact |

Inputs are the corresponding recordings in
[`tests/native/parity_inputs`](../tests/native/parity_inputs). The Luigi
Cyclone case reuses `drmario-down-air-contact-inputs.fmr`. These checks prove
the recorded paths, not every branch of every special. Workshop usability
and broader stock-character acceptance remain separate work.

The module layout changed for the landing-action event. Local Rook's module
was rebuilt and her installed package replaced; the collection still has one
installed Rook. No fighter assets or authored move source were changed.
