# Single source fighter integration checkpoint

Ordinary fighter projects and packages now feed the typed interpreter directly.
They carry owned asset facts, DATs, FMS, parameters and CPU data, with no character
DLL or generated-C payload. Workshop source checking is read-only; successful
script building atomically replaces FMS. Actual scene registration prepares a
separate package, publishes its asset tables and program together, and retains
the owner until actors, external leases and rollback history have drained.

The behavioral C exporter remains a separate test-only executable. It is absent
from the client and normal content tool. It is an oracle for existing comparison
fixtures, not the ordinary execution backend.

Real integration exposed two problems missed by preparation-only checks. The
animation importer had placed the compatibility tag at bit 24 after the DAT
decoder had converted it to the native six-bit field at bit 26. The corrected
import preserves all lower 26 flag bits, verified against actual stock imports
for both action and demo banks. State transitions also still required a native
Definition. They now validate the selected source package, data, program bank,
motion layout and live actor storage instead. Encoded Article factory calls now
route to the spawn handler before ordinary enum-range dispatch; previously their
large semantic IDs were mistaken for feedback operations.

Evidence from this checkpoint:

The native client, content tool and real-runtime fixtures build successfully.
All 32 focused interpreter/pool/lifecycle CTests pass, alongside the content
store, source packet and held-article backend CTests. Controlled primitive
fixtures explicitly separate selected source ownership from absent engine
families; they do not claim to load an actual package or render a character.

- `source-assets-registry-oct07-root/receipt.json`: 2,175 checks / 26 cases with
  real retail DATs, complete Mario source, 35 attribute imports, typed overrides,
  failure isolation and animation tag/flag preservation.
- `source-authoring-cli-oct07-root-second/receipt.json`: six real CLI cases,
  including clone, read-only checking, atomic source publication, failure
  preservation, independent authoring hints and canonical package verification.
- `workshop-source-script-oct07-final/receipt.json`: 14 checks through the exact
  Workshop hidden-process helper against the actual tool and copied Mario draft.
- `source-package-runtime-oct07-root-second/receipt.json`: 43 checks through the
  actual loader and engine/program table publication, rejected replacement,
  external leases and a completed Windows rollback-history frame. The fixture
  uses the real common DAT importer and creates no gameplay actors.
- `source-package-live-oct07-root-fourth/receipt.json`: four source-only Mario
  clones completed a hidden, muted 2,000-sample Battlefield match. All four
  interpreter owners stayed unfaulted; traced positions, velocities and animation
  scalars were finite, and the object census reported zero errors. Specials and
  cape Articles occurred. This workload did not select Fireball.
- `source-held-article-final-review/receipt.json`: 88 checks / 14 cases through
  the actual Item actor, VM and GObj lifecycle for generic held-article hooks.
- `source-installed-clone-roundtrip-review/receipt.json`: 42 checks for the real
  source-only installed clone lifecycle, including independent editing and
  rebuilding after the private author project is removed.
- `source-action-duplicate-roundtrip-review/receipt.json`: existing action editor
  isolation and relocation checks plus validated source package publication.

These receipts are under `A:/FreakMelee-test-archives/`. The earlier failed live
attempts remain there and are explicitly unaccepted; a clean process exit alone
is insufficient. The zero completed root-tick count in Mario's report is expected:
this script has no root `on tick` function. Its motion and lifecycle callbacks
execute independently of that diagnostic counter.

This establishes the Single source package route, not complete migration of the
roster or arbitrary content. Paired forms/companions, Rook's owned PNG/blueprint
resources and source resource-graph schemas remain open. The Workshop UI layout
is unchanged; full interactive preview editing and stock gameplay comparisons
remain separate acceptance work. Broad page tracking is still the rollback
oracle; complete explicit mutable snapshots and rollback timing are unfinished.
The one-stock match-end probe exhausted its 2,000 controller samples before
results/scene retirement, so match-end remains unaccepted for this new route.
