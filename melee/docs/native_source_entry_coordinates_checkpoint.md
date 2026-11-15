# Explicit move entry, hitbox coordinates and positioned articles

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

This interpreter batch closes three generic source preparation gaps without
changing fighter templates or deleting declarations to obtain a pass.

`entry explicit` is validated once, directly inside a named move, before its
callbacks, and requires an authored special entry. The interpreter already
executes the complete authored callback; the declaration adds no hidden resets
or accessory assignment. The real-library-linked metadata test passes 66 checks
across fourteen cases, including numeric motion/phase ownership.

`hitbox.current(index)` and `hitbox.previous(index)` expose separate writable
Float X/Y/Z coordinates. Their index is retained once and checked against the
four native capsules without wide-value narrowing. Writes preserve native
Float bits and do not advance history. `hitbox.set_world_position` follows
`ftColl_8007B8A8`: clear the joint and set the bone offset, without changing the
current or previous positions. Backend source guards retain the owned fighter
data and motion layout. Terminal namespace calls now reject during statement
compilation; a selector needs a scalar projection or an executable operation.

`articles.create_at_position` admits either six arguments or eight. The latter
supplies the collision group and creation value; the six-argument form defaults
both to zero. The twelve-argument velocity form is unchanged. Signed native-word
collision groups preserve their UInt32 representation; malformed or wider
values reject before construction. Descriptors are selected at preparation,
with pure descriptor data separated from frontend resolution.

The connected spawn cohort passes 270 checks / 33 cases through the actual
compiler, VM, actors, factories and extracted public constructors. It checks the
constructor's distinct current/previous positions and supplied creation value,
including negative zero. Assets, models, audio and collision leaves are
controlled. Receipt:
`A:/FreakMelee-test-archives/script-position-data-spawn-oct09-final/receipt.json`,
SHA256 `ee23cb3838ae968d800b564d0b5ed3a5888e42feed621e0fc152ecbe67c659c1`.
All 1,678 pinned inputs were unchanged during that proof. It ran before the final
unrelated terminal-namespace statement guard; factory bindings did not change.

The final coordinate cohort passes 2,063 checks / 286 cases with 84 VM calls
and 32 negatives. Its 253 comparisons comprise 168 raw Float field observations
and 85 extracted stock helper comparisons. It preserves the stock command guard,
XY-versus-Z-only branch and initial-zero previous-position behavior. The unchanged
existing state/damage fixture also runs through the current dependency closure:
11,344 checks / 3,250 cases, 3,207 native pairs and 3,250 VM calls. The final
namespace guard is included. Selected source package/data, stale-table records
and surrounding engine hooks are controlled; this does not establish
stage collision, rendering, history restore or complete Sheik behavior. Receipt:
`A:/FreakMelee-test-archives/script-fighter-hitbox-coordinates-oct10-final/receipt.json`,
SHA256 `ba809f53f7b516329c67cf9f4c4d83a6946de534d8062e3ff55a401a72a5f561`.
All 1,689 pinned inputs were verified unchanged.

## Shared verification and next source gaps

The final client, content tool and source-assets build passes, followed by all
eleven selected allocator, compiler, Item and metadata CTests. Standalone catalog
dependency maintenance updates 91 runners and two extractors; all 93 Python
files parse. This does not claim that all historical consoles execute.

The complete source census still passes Mario, Dr. Mario, Peach, Game & Watch
and Zelda preparation. Link/Young Link now stop at their mutable sword-trail
callback payload. Sheik reaches `hitbox.set_enabled`; Popo/Nana reach
`effects.spawn_on_part`. Later bodies and whole-fighter behavior remain
unaccepted at these boundaries. Census receipt:
`A:/FreakMelee-test-archives/source-entry-hitbox-spawn-census-oct10-final/receipt.json`,
SHA256 `6d5dd87479eb0aa8793475be5f4c1caae2cf271826e02be41d4ebcffab260655`.

No game window, rendering, audible launch or whole-match rollback acceptance
was required or claimed. The full project goal remains active.
