# Source entity lifecycle bindings

The direct catalog supports `articles.spawn_authored_projectile`,
`spawn_bone_attached`, `spawn_attached`, `owned`, `pause_owned`, `resume_owned`
and `remove_owned`; `items.held` and `items.pull`; and Item
`attachment_active`, `change_state`, `change_state_flags`, `activate_attached`,
`interact` and `spawn_food`.

`native_script_binding_spawn_resolve.c` selects Article declarations while
compiling the body. Both canonical identifiers such as `It_Kind_Mario_Fire` and
arbitrary declared names such as `CustomBolt` identify exact source owners.
Custom names become immutable source-local integer declaration identities;
they do not guess physical Item kinds. The prepared asset/program supplies the
physical kind. Helper, state/parameter, native namespace and shared-constant
collisions are rejected. Dynamic kind expressions and bare numeric targets are
rejected. A custom identity cannot enter parameter defaults through an
unindexed/default expression arena.

Factory call semantics encode operation and owner in native domain
`0x02000000`. The common descriptor and call-layout lookup understand those
semantics. Raw factory calls are rejected if load-time selection has not run.
Execution performs no source-text or declaration-name lookup. `items.pull`
selects a numeric recipe from the immutable prepared root table and verifies
the caller's exact bank. All storage and temporary guards use fixed pools.

The runtime invokes the substantive `nativeScriptItemSpawn_*` factories and
native Item state/removal primitives. It retains caller scene/pool generation,
program, storage and layout/parameter identities. Owned article operations
capture a target's pool/game generation and exact program/storage view before
effects, require its retained creator and current owner to match the caller,
and preserve that target through pause/resume. Removal intentionally consumes
the target and validates the caller afterwards. Source Item interactions accept
an exported transient engine token, clear references through actual
`it_8026B894`, and validate that target before publishing its Boolean result.
No authored native callback or generated-module Host is used.

Numeric creation-ID handles preserve existing authored scalar state. Attached
spawn returns Int32, assignable to Float state. Publication is limited to
`0..16,777,215` so Float conversion cannot round a new handle into another live
ID; supplied Float handles outside that range or fractional/nonfinite handles
are diagnosed. Missing, reflected, retired or differently owned/programmed IDs
do not select a target. These raw numeric IDs remain scene-local: they cannot
encode a full generation and cannot distinguish hypothetical counter wrap or
explicit creation-ID reuse. This slice does not add persistent Object state or
an unbounded provenance ledger. Factory failures remain diagnostics and do not
undo completed native side effects.

`items.held` accepts ordinary engine Items as well as interpreted ones. It
checks the actual hand pointer, Item entity, mutual owner relation and logical
removal flag; deferred native storage still being registry-live does not report
an already removed Item as held.

Connected evidence:
`A:/FreakMelee-test-archives/script-spawn-bindings-oct07-i/receipt.json` reports
17 cases and 135 checks. Authored expressions pass through the production
source compiler, bytecode, VM, binding catalog, factories, actors and extracted
actual constructor/scheduler/state/removal bodies. The fixture also uses actual
creator capture/resolution, part mapping, pause/resume, reference clearing and
attachment activation bodies. It covers two declarations sharing a physical
kind, custom source names, Float handles, created/destroyed callbacks with root
state writes, holdable pull/food, ownership loss, caller retirement, deferred
held-item removal, malformed handles and load-time rejection cases.

The fixture's immutable specs/assets are manually supplied. Model, graphics,
audio, collision and platform leaves remain controlled; this is not package,
billboard, live rendering or whole Mario/Rook parity acceptance. The separate
factory fixture covers all factory routes and transition-stage failures.

Recipient-qualified factory calls such as `creator.articles.spawn_*` remain
unsupported: the receiver projection currently reserves only twenty payload
bits and cannot represent the full owner-encoded factory semantic. Root
Fighter factory calls used by Mario/Rook are supported. Broader source package
publication, source holdable visual preparation, effect operations and other
unported template families remain separate closure work.
