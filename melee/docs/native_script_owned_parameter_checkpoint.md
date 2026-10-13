# Owned parameter import and Jigglypuff runtime checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The shared parameter importer now replaces DAT-offset declarations with typed
literal `parameter` declarations in their original fighter/article scope. It
also writes matching editable sidecar values. Previously it removed declarations
entirely, which the generated-C path tolerated but the ordinary interpreter
could not index. This fixes the actual importer used by both the content CLI
and Workshop, rather than adding a test-specific conversion or runtime alias.

Converted scripts contain no tuning offsets. Root reads remain `attrs.NAME`
or `program.attrs.NAME`; article-local reads use `item.params.NAME`. Imported
float defaults retain nine significant digits for binary32 round trips. Typed
sidecar overrides must name an existing declaration. Output writes remain
bounded. See the [parameter reference](scripting/language.md#fighter-attributes).

The focused jump/costume runner now creates and verifies a source package,
without generating a DLL. Its subprocesses remain hidden and game audio muted.
The blue costume explicitly selects visibility variant 2. It verifies ordinary
actor admission, requested tick coverage, absence of faults and native trace
parity, with all five aerial jumps required in both directions.

## Results and limits

- Fresh clone/import/save/package/verify/accept succeeds in an isolated test
  store. The converted Jigglypuff source prepares all **192** bodies with
  **zero DAT tuning imports**, using its literal defaults and sidecar overrides.
- Three recorded stock/ordinary-interpreter pairs match exactly: right-facing
  and left-facing five-air-jump sequences, and the worn Bunny Hood sequence.
  Each covers **900** controller samples and **1,800** traced fighter states.
  Actions, fighter poses/collision digests, items, damage and RNG agree. The
  coverage check confirms all five jump motions and facing changes, plus the
  actual worn item in the Bunny Hood case.
- The fourth, blue-hat Rollout pair completes **1,200** samples without script
  faults. Its accessory digest differs on 1,199 samples; other traced fighter
  fields agree. **This costume comparison is not accepted.** Attachment
  pose/material/dynamics behavior needs further inspection; the digest alone
  does not explain the visual difference.
- The initial batch's health assertion incorrectly required a completed global
  `on tick` callback. Jigglypuff has none: it uses motion and event callbacks.
  Requested tick count is positive and faults are zero. A separate pinned
  post-analysis corrects only that assertion and runs the jump coverage checks
  on the existing logs. No binary, package, input recording or comparator was
  changed, and the blue-hat failure remains recorded.
- The connected capture/lifecycle cohort still passes **102,372 checks /
  9,615 cases**. The importer/reference-expression cohort passes **76 checks /
  11 cases**, including scoped literal declarations. Its generated-C expression
  checks are a reference oracle, not gameplay evidence.
- Seven Windows targets build; reference script and parameter CTests pass.
  Fresh roster preparation remains **20/26 forms**. These results do not
  establish complete Jigglypuff special, shield-break or whole-match rollback
  acceptance. No normal installed collection or user draft was modified.

Evidence, independently rehashed:

- `A:/FreakMelee-test-archives/purin-interpreted-jump-oct10-b/post-analysis.json`:
  SHA-256 `22c175d320927a29544b5e0b670d27e56904dec98c618cde2d223399e0a53a45`.
  Its linked original manifest pins binaries, copied project assets, FMS,
  parameter text and recordings. The documented delta is the health assertion.
- `A:/FreakMelee-test-archives/owned-parameter-declarations-oct10-capture/receipt.json`:
  SHA-256 `46041147f7b1dd0360fba5e94b41e2465ba435e6245e2d40821befb12d835264`,
  **1,831** pins.
- `A:/FreakMelee-test-archives/owned-parameter-declarations-oct10-reference-final/receipt.json`:
  SHA-256 `b3b50a189c560b9992de303b084fc811fc6b6980768f9f5348b421a4589daebe`,
  **1,558** pins.
- `A:/FreakMelee-test-archives/source-full-roster-audit-oct10-owned-parameters/receipt.json`:
  SHA-256 `72f9ce28f6efa3973249ffa4b0e20d3247791bcfa5e66c7164233daf0c6a7012`,
  **216** pins.
- `A:/FreakMelee-test-archives/owned-parameters-independent-pins-oct10.json`.

The overall goal remains active. Resolve the accessory mismatch, remaining
fighter resource/package prerequisites, full clone behavior coverage and compact
whole-match rollback acceptance in grouped cohorts.
