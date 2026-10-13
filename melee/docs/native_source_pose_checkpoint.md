# Direct interpreted pose operations

Implemented 2026-10-08. Named owned graph metadata, closed selector values and
all eight pose operations now work through ordinary source compilation and the
typed VM. They call the existing shared Melee pose functions directly; no
generated C, borrowed fighter handler or additional animation policy is used.

## Source and ownership

`pose_graph Name slot N path "child/path"` must be root-only, precede executable
blocks and match an exact owned asset tuple. The compiler prepares at most 16
symbols mapping declarations to actual facts rows, independent of row/source
order. Missing facts, tuple mismatches, collisions with declarations/constants/
language words, unknown paths and oversized tables fail the candidate. Package,
Fighter-program and Item-program validation all recognize the prepared table.

`Pose.live`, `Pose.blend` and named graphs are nonnumeric closed selectors.
They lower to typed constants and retain their types through VM moves. Numeric
casts, arithmetic, comparisons, scalar declarations and numeric graph/destination
arguments reject. Native marshalling checks exact selector type and bounds before
the ordinary numeric path. Graph arguments carry only an owned row index, never
a native pointer; native calls verify membership in the selected program.

Pose operations use the generic source-only descriptor flag. Receiver traversal
does not expose them, preventing a caller's graph selector from being interpreted
in another fighter's bank through `creator.pose` or another recipient relation.
Articles/effects cannot use this source-fighter surface. Recipient helpers can
still implement their own behavior using their own declarations.

## Native behavior

The eight operations are documented in [pose API](scripting/poses.md). Clip and
raw-part indices preserve their integral width/sign through marshalling and
reject out-of-range values before narrowing. Frames and two independent weights
are finite F32 inputs; weights are not clamped, normalized or reconstructed.
Loading a clip does not request/evaluate it or enter a motion. Full-weight blends
remain actual blends, preserving the shared copy/blend differences.

The dispatcher captures the physical pool reference, logical actor/storage,
selected package/program, exact parameter/layout owners, rig parts and both
skeleton heads. Existing owned clip/graph resolution and substantive topology
validation precede each shared operation. Native pointers remain local to one
synchronous call. Rechecks after costume lookup, animation loading/evaluation
and copy/blend prevent further native reads or output publication after retirement
or replacement. There is no parsing, allocation or name/hash lookup in ticks.

The shared bounded path validator moved into `native_fighter_pose_path.c` so the
compiler can use the real implementation without linking unrelated engine pose
operations. Call sites still call the same substantive function directly. Console
fixtures explicitly reject reached unavailable native pose calls.

## Evidence and remaining acceptance

Receipts are in `A:/FreakMelee-test-archives/`:

- `source-pose-metadata-settled-root/receipt.json`: 237 checks, 57 compile attempts;
  actual compiler/table/resolver and path implementation, controlled owned facts.
  Includes reordered rows/declarations, sixteen-row boundary, foreign contexts,
  reused-bank clearing and byte-exact preservation of the accepted bank.
- `script-pose-catalog-oct08-marshalling-final/receipt.json`: 139 checks; actual
  expression/value/statement/bytecode/VM/native argument marshalling. A controlled
  graph resolver supplies row7; a deliberately rejecting native leaf captures the
  exact closed types and raw U64 start. No pose rendering is simulated as success.
- `script-pose-runtime-settled-root/receipt.json`: 1,858 checks, 208 cases and 182
  retirement/replacement mutations across fourteen destination combinations.
  Actual dispatcher/catalog/source guards/physical lifetime/pool; selected asset
  views, topology validator and shared pose math are controlled.
- `source-pose-owned-preflight-settled-root/receipt.json`: ordinary content-tool
  source-check accepts an eight-operation helper against real copied Sheik DATs
  and owned graph facts, with reversed source declaration order. One function,
  54 instructions, no imported parameters; original draft files unchanged.
- `source-form-template-pose-preflight-oct08-root/receipt.json`: full Zelda still
  rejects `callbacks.track_reflect_hit` at103. Full Sheik passes the graph schema
  and next rejects entity metadata at1543. These templates are not fully migrated.

Client/content-tool and affected console/package targets build; all 36 selected
regression CTests pass (`.local/source-pose-unified-build.log`,
`.local/source-pose-ctest-final.log`). This establishes the generic pose surface,
not live Chain animation fidelity, complete paired gameplay or snapshot timing.
User drafts/installations were not changed and no game was launched in this batch.
