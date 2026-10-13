# Saved actor/GObj ownership preflight

`nativeSnapshotActorGraph_Validate` consumes four immutable, validated pool views:
GObjs, fighters, Ground records and items. It checks their actual pool identities,
typed record sizes and shared scene lifetime, then checks both directions of
ownership. Each saved actor must point to a saved occupied GObj with the expected
classifier, userdata tag, physical actor address and destructor. Each saved
fighter/item GObj (or GObj tagged with one of these actor destructors) must point
back to its saved occupied actor. Ground roots are identified by their actual
native Ground destructor, since other stage GObjs can also use userdata tag 3.

All fields are read from saved records through memcpy; roots are mapped to stable
slot addresses and looked up in candidate records. Current occupancy, generation
reuse and current actor bytes do not decide candidate graph membership. The
validator does not allocate, write a pool, traverse arbitrary candidate pointers,
run callbacks, restore objects or simulate anything. Source constructor/destructor
behavior is unchanged.

The offline same-boundary self-restore census prepares these views from its own
fully validated captures and requires actor-graph agreement before any Load.
Its captured GObj component already proves its internal scheduler/link topology.
This new check supplies actor/root relationships that individual payload owners
could previously only check against the live timeline. It is one preflight part,
not a general history loader or proof that all components belong to the same
historical frame. The diagnostic still uses its current-boundary-only contract.

Evidence under A:/FreakMelee-test-archives/:

- actor-graph-oct11-owner-b: actual generic pool/view and graph code with controlled
  typed actor/root construction and aborting destructor endpoints. 115 checks,
  ten malformed relationships, zero callbacks. An immutable older candidate is
  accepted after both its live root and fighter slots are reused. Wrong owners,
  tags, callbacks, classifications, absent/crossed roots and duplicate/orphan
  roots reject; missing views/retired bindings also reject. Live backing and
  descriptors remain byte-identical. A synthetic non-Ground stage-camera row
  exercises userdata-tag sharing without following its unrelated payload.
- actor-graph-oct11-eight: stock and interpreted eight-actor runs each pass ten
  actor graph preflights, all 34 component captures/validations and 340 actual
  same-boundary Loads/byte-identical recaptures. Prior/current and diagnostic
  off/on gameplay traces agree.
- actor-graph-oct11-stages: Fountain, Green Greens and Corneria each pass 4,800
  rendered old/new samples, forty actor preflights and 1,360 Load/recapture pairs.
  Fountain exercises its actual reflection-camera role alongside Ground roots.
- actor-graph-oct11-independent-verification.json: final source/executable/log
  pins and prior/current eight-actor traces verified. Total live preflights: 140;
  total Load/recapture pairs: 4,760. Four native build targets and focused pool,
  ObjAlloc and GObj-consumer CTests pass.

Class/model spans and types, animation/article/VM/particle relationships, identity
and callback registries, remaining world/resident/HSD/HUD ownership, aggregate
candidate publication and changed-state rewind/resimulation remain open. Production
retains the broad page oracle; no completed rollback replacement or latency bound
is claimed.
