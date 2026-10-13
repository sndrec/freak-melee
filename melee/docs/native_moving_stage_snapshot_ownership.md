# Kongo Jungle and Icicle Mountain private state

The existing stage-hazard component now includes two additional actual records.

Kongo Jungle owns its parameter pointer, two bank-joint borrowers and all fifteen
joint-motion entries: joint IDs, control flags, joint borrowers, initial/current
angles and motion values. The original fifteen-entry initializer is moved
without changing any value or implicit zero. Stage functions access the new
record directly. The private parameter pointer formerly lived in
`grkongo.static.h`; its storage is removed there and defined in the actual
record. Included private headers need auditing too, not just top-level source
declarations.

Icicle Mountain owns its parameter pointer, three extra-row handles and the
six-entry row table. IDs and all initial numeric values are preserved. Current
versus initialization leaves those three handles null; the extra rows are used
by the non-versus stage variant. The connected fixture exercises populated
handles and generation restoration, but that is not a live adventure-mode
acceptance test. Live evidence is scoped to selectable versus stages.

## Reference and restoration rules

The snapshot remains 2,800 bytes: both records fit inside the existing active
stage union, and Kongo's seventeen joint claims fit the existing twenty slots.
Kongo joints are model borrowers without separate actor handles in this record;
they must be captured and validated even when the corresponding actor-reference
slot is empty. Icicle Mountain's non-null actor handles use paired GObj/Ground
generation claims and the existing reciprocal role/destructor checks.

Class identities are admitted from initialized named stage joints and private
Kongo joint borrowers. Cached backing claims refresh on attachment changes.
Class backing is an epoch/block/span claim, not an occupied-object generation.
Class/pool companion candidate membership and graph agreement remain aggregate
responsibilities. Restore checks every live actor and joint before writing any
private value. This extends the existing active-stage lease rather than adding
an independent state representation or a forwarding layer.

The real stage bodies retain their original decisions, mathematics, RNG calls
and callbacks. Only direct storage names and the local joint-motion type name
change. Parameters remain borrowed; their payload ownership is not established
by copying the pointer. Other private stage data, variable allocations, other
globals, multi-heap/resident closure and complete aggregate rollback remain
open. The production page oracle remains active.

## Evidence boundaries

The connected fixture links actual class/Ground owners and snapshot serializers
with controlled GObj backing, actor/model initialization, parameter identities
and an idle predicate. It checks original initializer values, sixteen repeated
controlled evolutions for both shapes, late joint/handle candidate corruption,
changed attachments, class companion ordering, actor-generation reuse and
activity/epoch refusal. It does not simulate real stage animation or hazards.

The live cohort separately compares rendered CPU gameplay on Kongo Jungle,
Icicle Mountain and selected existing owner cases. Capture/Validate observations
are not live Load or whole-match performance acceptance.


Final receipts under `A:/FreakMelee-test-archives/`:

- `moving-stage-oct11-owner-b`: 113 additional moving-stage checks and thirteen
  malformed candidates; earlier 178 hazard and 76 Brinstar checks also pass.
  All seventeen Kongo claims and the populated Icicle Mountain actor shape are
  exercised, with initializer checks, repeated controlled evolution, attachment
  changes, late candidate rejection, class payload restoration and generation
  reuse. This is owner restoration with controlled construction, not live stage
  or adventure-mode resimulation.
- `moving-stage-oct11-stages`: five old/current rendered CPU scenarios, 2,400
  samples each: Kongo Jungle, Icicle Mountain, Brinstar, Corneria and Final
  Destination. Populated gameplay traces match exactly and all twenty shared/
  private capture/validation boundaries pass per scenario. No live Load.
- `moving-stage-oct11-eight`: stock and independent interpreted eight-actor
  scenarios pass all 28 components at ten boundaries each. Diagnostic-on/off
  and previous/current populated traces match exactly. No live Load.
- `moving-stage-oct11-gm-a`: existing connected control/player/camera-target
  restore fixture passes; no actual moving-stage startup graph is constructed.
- `moving-stage-oct11-independent-verification.json`: owner/frozen/live pins,
  actual compiled sections, all 121 donor bodies and both exact initializer
  tables independently verified, including removal of the private-header
  parameter storage. Four build targets and focused GObj-consumer CTest pass.
