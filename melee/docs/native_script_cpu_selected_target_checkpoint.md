# Interpreter access to the selected CPU target

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

`cpu.target_id` exposes the planner's current Fighter selection as a read-only
U32 creation ID. Null selection returns zero. Physical registry membership,
scene, class, userdata, backlink and source/program/storage identity are admitted
before publication. The getter never dereferences an unregistered selected data
pointer, selects another opponent, allocates or draws RNG. The ID describes the
current physical selection at read time; it does not reconstruct a historical
generation from the CPU planner's raw pointer.

Yoshi and Ness use this ID with the existing generic position, airborne,
collision-line and island queries. Their old CPU-specific target projections
and shared-floor shortcut are removed from the test exporter and current API
documentation. Ness's composed reference inputs were migrated at the call sites
as well. `int(cpu.target_id)` preserves all 32 bits in the existing Int state/
local representation; generic target queries interpret the same creation-ID bits.
Unsigned owned state was not added as an incidental requirement of this change.

The connected CPU cohort passes **31,213 checks / 1,291 cases / 3,925 native pairs /
1,567 VM calls**, and the command/selector cohort passes **7,260 checks / 612 queue
pairs / 80 selector pairs**. New cases cover null, self, foreign authored and
stock selections, high-bit IDs, malformed class/backlinks, unregistered payloads,
GObj reuse with a different Fighter payload, invalid source admission and
read-only/source-only compiler rules. All 1,726 inputs were independently
rehashed unchanged. Synthetic Fighter payloads and package publication remain
controlled; this does not prove complete CPU scheduling or gameplay parity.

The five native targets and test-only reference exporter build. A minimal query
using the new target ID and generic position/airborne/floor methods generates C
and passes Clang syntax checking. Whole Yoshi/Ness C export still stops earlier
at their unrelated Article attributes (lines 1108/1716); no full-fighter oracle
export or runtime result is claimed. Ordinary source execution has no C fallback.

The nine-source preparation cohort keeps Captain Falcon, Ganondorf, Marth, Roy
and Luigi passing. Yoshi advances from its old target operand to
`cpu.travel_frames` at line 1574 (the original checkpoint mislabeled this
as the ranged query; a later line-by-line source check corrected it). Ranged/travel CPU policy, remaining capture
hooks, owned Article integration and full fighter acceptance remain open. Ness
call-site migration is not whole Ness source/package acceptance.

Evidence under `A:/FreakMelee-test-archives/`:

- `script-cpu-target-id-oct10-final/receipt.json`, SHA256
  `33813a241f91084f8f3d9ea3050c19f175f4859f05ee543a0f55c3017b2684b7`.
- `source-cpu-target-id-cohort-oct10-final/receipt.json`, SHA256
  `6e2e4d10ab495ea5cc2d736553a10394046f3df44fae7cbe0b160728a9db3777`.
- `cpu-target-reference-export-oct10-final/receipt.json`, SHA256
  `820a8665be31b7c6b58b2376a2a9991f74317ebfdff0d1e7de132c26265f2504`.

No game was launched or installed assets changed. Compact whole-match rollback
remains unfinished; the project goal stays active.
