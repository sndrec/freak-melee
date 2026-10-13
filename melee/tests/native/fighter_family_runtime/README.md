# Fighter family orchestration source fixture

The scoped console matrix passes **109 cases / 93,068 checks**. Final receipt:
`A:/FreakMelee-test-archives/fighter-family-runtime-live68-member-final/receipt.json`.
No game process, real member DAT import or player entity creation is exercised.

From the repository root, reproduce with a fresh output directory:

```powershell
python tests/native/fighter_family_runtime/run.py --snapshot .local/baseline67 --output A:/FreakMelee-test-archives/fighter-family-runtime-live68-reproduction
```

`--source` optionally selects another actual runtime source file. The default
is the live `src/melee/ft/native_fighter_runtime.c`; `--snapshot` selects the
immutable compiler, headers and actual package codec. `--extract-only` saves
an explicitly untested extraction. Outputs must be fresh; do not register a
fixed CTest output directory without a deliberate rerun/output policy.
The live `MemberId` operation is newer than67 and compiled explicitly as
recorded below; this is not a claim that it was present in frozen67.

## Production code exercised

The runner requires actual function definitions and extracts complete bodies:

- `release_slot`, `release_family`, `prepare_family`;
- `nativeFighterRuntime_ValidateCached`, `nativeFighterRuntime_StageCached`;
- `bind_family`, `nativeFighterRuntime_ActivateStaged`;
- `nativeFighterRuntime_CharacterSoundBanks`, `nativeFighterRuntime_Load`;
- current `nativeFighterRuntime_MemberId`.

The complete actual family/actor storage declarations are extracted too. The
actual frozen `native_fighter_package.c` and actual `nativeContent_Hash` body
execute canonical packet building/parsing and Windows CNG hashing. Test
packets contain controlled placeholder member payloads, not valid DATs/DLLs.

The receipt records source, fixture, generated extraction and selected SDK
input hashes, compiler/freeze hashes, commands and captured build/run output.
Selected compiled SDK headers, package source and compiler are checked against
the frozen manifest; the runner does not independently rehash every transitive
SDK include. The whole live runtime differs from frozen67 by the additional
`nativeFighterRuntime_MemberId` function. All nine extracted function bodies and
the extracted storage region were separately verified identical to frozen67.
The additional tenth body is the actual live function, now explicitly tested.
The receipt records per-function correspondence. Actual frozen
`native_callback_lifetime.c` is linked for the new query; its source hash also
matches the current live lifetime implementation.

## Matrix

- Single, AlternateForm and SimultaneousCompanion; four families, eight distinct
  actor IDs for paired modes; both member binds precede one player mapping.
- Every paired member slot failing before allocation, after partial allocation
  and after complete controlled import; exact family/package cleanup with no
  publication or active resource retirement.
- Missing module, stock-dependent Definition and missing alternate-form
  completion hook; malformed packet, installation/default-root/read failures
  and absent common-parts bootstrap.
- Detached validation leaves active resource ownership intact; failed staging
  preserves the outgoing family, imported buffers and published mappings.
- Duplicate IDs, count/null/range rejection; old staged cleanup; activation
  count mismatch and active-scene rejection; whole-family Load collisions.
- Member-zero-only requested Definition, both member preflight flags, retained
  load-context invalidation on release, staged sound union preceding outgoing
  sound and active union after publication, missing/cleared family sound zero.
- Immutable member lookup across Single/both paired modes and player role swaps;
  full high-bit creation IDs; wrong player/index/role/kind/data/active family or
  member rejection; a same-player registry lookalike outside the selected roles
  cannot become the caller. Recycled live member addresses resolve their current
  creation ID. An unlinked `PAGE_NOACCESS` caller or selected counterpart is
  rejected without reading its object storage.

The fixed allocation ledger rejects double/foreign frees and checks that all
member allocations are released before the one retained family packet. The
93,068 assertion count includes ledger checks; it is not a count of independent
asset-import scenarios. No confirmed orchestration defect was found.

## Controlled boundaries and limits

`prepare_member` is a substantive controlled import boundary: it records real
retained packet spans/member indices/Definition selection/kinds/preflight,
allocates independently owned member buffers, and injects failure phases.
Actual rig, animation, model, module/schema and CPU import are not linked here.
`bind_member` records publication ordering rather than mutating engine tables.
Store reads/default roots/installed membership, common-parts loading, resource
retirement, library close and player registration are counted boundaries.
Extracted `release_slot` allocator calls use the fixture's allocation ledger.
MemberId uses actual live-list Snapshot, with controlled player-character/role
getter tables. It proves query validation and immutable index selection against
those tables, not actual player-role swap mutation or fighter entity creation.

This proves family orchestration and ownership ordering within those boundaries.
It does not prove actual `nativeFighterRuntime_Clear` engine-table reset,
costume preflight, live form transfer, results/death behavior, companion AI,
rollback or gameplay. The historical actual-asset gate is documented in
`real_asset_probe.md`. Its DLL/paired-packet mutator and runner were retired when
ordinary packages moved to the Single source-only protocol. Archived results do
not establish current source-family or paired gameplay acceptance; current packet
and runtime acceptance uses the source-package fixtures.

## Preserved history

The paused `fighter-family-runtime67-wip` receipt is extraction only. The first
executable attempt (`fighter-family-runtime67-first`) failed fixture compilation:
missing common-parts declarations and Windows `near`/`far` macro interference.
The fixture now includes the actual common-assets header and undefines those
macros before Melee headers. No production fix was required.
`fighter-family-runtime67-second` passed 82 cases / 59,154 checks; the final
matrix adds meaningful active/missing/cleared sound-union checks, producing
82 cases / 59,171 checks at `fighter-family-runtime67-final`. The requested
MemberId extension then produced the current 109 cases / 93,068 checks. Its
first compilation failure (fixture used a nonexistent character-enum spelling)
is retained at `fighter-family-runtime-live68-member-first`. Earlier output
directories are preserved.

## Constructor CPU history hint (SDK71 extension)

The extraction includes actual `nativeFighterRuntime_CpuHistorySource` alongside
strict `nativeFighterRuntime_MemberId`. The new hint can resolve a published
counterpart before its caller is published, checking actual active family,
member address/index, owned imported data, player selection and counterpart
kind/data/identity. MemberId still requires its source in a Player role.
Healthy first-before-second construction, secondary-before-publication, role
swaps, Single/Alternate/Companion families, high-bit/all-bit identities and
wrong/missing/retired source/partner cases run against the actual live registry.
Former unregistered source/partner pages can be inaccessible.

Root live-helper run with immutable70 compiler/headers passes149 cases /
118,080 checks at A:/FreakMelee-test-archives/family-history71-root/receipt.json.
All other extracted runtime bodies are byte-identical to frozen70; the new
helper is explicitly absent there. This verifies the resolver within the
existing controlled member import/Player getter boundaries. Actual CPU history
copying/generated policy is separate evidence in `../cpu_early_policy`.
