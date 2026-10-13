# Aurora texture identity snapshot

The 36th component owns Aurora's process-lifetime texture and TLUT identity
counters. `NativeTextureIdentitySnapshot` is eight bytes: two uint32 values.
`GXGetTextureStateRegions` supplies the actual two storage addresses, in texture /
TLUT order. Their shape and independence are checked before reading or writing.
The serializer has no live mirror, versioned format, allocation or callback table.

Original source allocation assigns successive identities to newly initialized
texture and TLUT objects. These persistent counters advance during drawing, so
restoring only model/particle state can leave the next identity sequence on the
newer timeline. The broad page oracle already included them as exact regions;
the compact component now captures that same state directly. Aurora code and its
identity-allocation decisions are unchanged.

Save/Validate/Load require the actual GM snapshot idle boundary and an idle script
runtime. They reject wrong-sized/overlapping buffers or malformed region shape
before any output/state write. Load copies both counters only after preflight.
All counter bit patterns are preserved, including zero: the original allocator
reports overflow when asked for an identity after the UINT32_MAX allocation has
advanced its next counter to zero. Snapshotting does not change that policy.
These fixed process-lifetime scalars need no scene rebinding or generation claim.
Buffers must also remain independent of other owners when used in an aggregate.

This owns identity counters only. Texture-object records, immutable image payloads,
GPU resources, renderer queues and cache/regeneration policy are separate. Matching
identities does not prove complete graphics recovery or authorize skipping drawing.

The fixture compiles the actual source-extracted Aurora increment and region-query
function bodies; only the C ABI declarations and aborting FATAL diagnostic leaf
are supplied by its harness. Idle predicates and starting counter storage are
controlled. It compares 128 texture/TLUT identity pairs across restore and repeat,
checks refused output overlap/sizes/activity without writes, and exercises the
original UINT32_MAX-to-zero exhaustion boundary without invoking the abort leaf.
No GPU/texture construction occurs in the fixture. Live rendered cohorts exercise
the linked production getter/counters separately. No performance deadline is
claimed from the fixture or the small sample of component timing.

Evidence is recorded under A:/FreakMelee-test-archives/ with the
texture-identity-oct12 prefix. The production page oracle remains unchanged;
complete aggregate prevalidation, compact resimulation and final timing remain
unfinished.

Final evidence:

- texture-identity-oct12-owner-a: 162 assertions using the actual serializer and
  extracted source counter/query functions. Repeated 128 identity pairs match;
  refused activity/buffer cases preserve output/state and exhaustion is retained.
- texture-identity-oct12-eight-a: stock and interpreted eight-actor cases each
  pass all 36 components, 360 same-boundary and 36 older Load/recapture pairs.
  The texture identity component captures/restores eight bytes; source/exe pins,
  previous/current and diagnostic-on/off traces are exact.
- texture-identity-oct12-stages: Fountain, Green Greens and Corneria each pass
  4,800 rendered samples, 1,440 normal and 36 older pairs, with exact old/new traces.
- texture-identity-oct12-independent-verification.json: final source/executable
  pins, eight-pool coverage totals, 36-owner recaptures and traces verified. Four
  native build targets and focused allocator/pool/GObj-consumer CTests pass.

Remaining unclassified raw bytes are 149/151 (stock/interpreted eight-actor),
175 (Fountain), 188 (Green Greens), 212 (Corneria); required registered pool state
still has zero differences. This source ownership closes the persistent identity
counter gap, not every remaining transient/persistent state or graphics issue.
The shared aggregate capture/load extraction and experimental rendered
restore-and-resimulate path are the next integration tasks, retaining the broad
oracle as an independent reference until final acceptance.
