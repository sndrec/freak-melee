# Source spawn bindings

This hidden console cohort compiles complete source expressions through the actual frontend, prepared descriptors, bytecode, VM, Fighter/Item actors, storage and fixed GObj pools. It executes the actual source factories and extracted public Item constructor/scheduler bodies. Immutable program specs and asset descriptors are controlled; model construction, graphics, audio and collision leaves do not establish DAT import, rendering or gameplay parity.

The positional batch covers the six-argument default and eight-argument `articles.create_at_position` forms, plus the existing twelve-argument velocity form. It checks exact constructor current/previous positions, zero or supplied velocity, initial collision, original creator, full-width collision words and copied creation values. The native initializer starts `Item.pos` at `SpawnItem.prev_pos`; `origin_at_owner` separately changes the constructor's current position. The fixture records the actual public constructor inputs rather than assuming the controlled collision leaves perform a position update.

Real authored Spawned/Created callbacks observe creation value zero followed by the supplied value, including negative zero. Compiler negatives reject seven/nine arguments and a Float collision group. Direct runtime negatives reject Float/wide collision words and malformed flags without changing the result or constructing a model. Existing attachment, projectile, holdable, removal, recipient and caller-retirement cases remain in the same cohort.

Run with a frozen clang executable and a fresh private A: directory:

Explicit projectile coverage includes `articles.launch_at_position` and
`articles.launch_projectile` through real source calls, owner resolution, VM,
actors and the actual shared projectile factory/constructor. Fourteen direct
factory/binding comparisons cover wrapped angles, positive/negative speeds,
states and enabled/disabled initial velocity, comparing complete constructor
inputs, setup order, native trajectory/lifetime/state/origin fields and authored
Launched results. The positional call retains its supplied facing and all32
collision-group bits; the bone form forwards its local offset, derives facing
from world angle and uses variant/group zero. The transform leaf records its
input and applies a controlled translation, not a real skinned joint matrix.

Compiler negatives reject Float collision groups, missing arguments and dynamic
kind selection. Runtime negatives reject owner/signature/type/width/nonfinite
inputs or an absent article state before construction. State, appearance, debug
and update retirement injections stop setup and leave output untouched while
retaining the already-created source Item. These are real factory/VM guards;
asset publication, rendering and unrelated platform leaves remain controlled.
Newer unselected article-release and JObj-class leaves abort if called, rather
than providing invented successful behavior.

```
python tests/native/script_spawn_bindings/run.py --compiler A:/path/clang.exe --output A:/FreakMelee-test-archives/fresh-output
```

The receipt pins sources, headers, compiler and generated boundary inputs. This is a controlled constructor/lifetime proof; it does not claim complete Popo/Nana preparation or game acceptance. Numeric creation handles remain scene-local and cannot distinguish raw-ID wrap/reuse.

The projectile declaration also uses a shared stock ItemKind name. Its compiled
Article selection must use the declaration owner encoded in the call semantic,
while the same identifier remains the numeric ItemKind in ordinary expressions.
Both world and bone launches, and the direct runtime launch matrix, cover this
case. A same-bank descriptor deliberately uses a different physical item kind
to detect accidental selection by the literal.
