# Interpreted smash entry and Ness preparation

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The October 10 batch integrates the existing generic `up_smash_enter` and
`down_smash_enter` declarations with the production Fighter hook table and
common input dispatch. These Void callbacks replace accepted smash entry,
after held-item throwing priority. Up-smash's ordinary and NoD0 paths both use
the replacement. A source fighter without the hook keeps common entry; stock
actors retain their stock dispatch. No Ness-specific engine API was added.

## Evidence

- Seven Windows targets build; the two focused script/parameter CTests pass.
- The frozen linked VM fixture passes 16,154 checks across 132 cases in four
  processes. Complete common input bodies are extracted from current source;
  C-stick intent, item classification, throw and default-entry leaves are
  controlled. Real source-owned Fighter callbacks execute through the VM.
- The fixture checks 432 dispatch inputs across source-owned and raw actors:
  normal and NoD0 up-smash, down-smash, timely/late attack, C-stick/tilt profile,
  neutral/special input and held-item priority. Full Fighter bytes are compared
  after dispatch. Profiles with and without replacement hooks are covered.
- Previous actual native HSD graph/controller comparisons and Pages restore
  regressions remain in the cohort. These are controlled component tests,
  not a full Ness match.
- The canonical Ness audit accepts 418 functions, 92 imports and 10,658
  instructions with zero diagnostics and complete metadata/body coverage.
  The read-only stock source census accepts 25/26 forms; Samus remains.
- A private Ness packet with real DAT/CPU assets and all eleven articles
  builds and verifies. A placeholder portrait is used only to construct the
  packet. Actual ISO-common import, staging, table publication, failed-staging
  isolation and retirement barriers pass 43 checks without a native fighter
  definition. This fixture deliberately creates no actors.

## Receipts

Under `A:/FreakMelee-test-archives/`:

- `source-smash-entry-vm-oct10-a/receipt.json`
- `source-roster-smash-hooks-oct10-a/receipt.json`
- `ness-smash-package-oct10-a/receipt.json`
- `ness-smash-publication-oct10-a/receipt.json`
- `ness-smash-hooks-oct10-final/receipt.json` records closure pins.

Private packet ID:
`4836f35a7b87afc873b316001332774075c3f35d5a4f869928c54216dfce6909`.
No installed collection was modified. All tests are console-only and silent.

## Remaining acceptance

Ness's live actor construction, Yoyo smash startup, article/effect playback,
full stock/script behavioral comparison and rollback through those operations
still need verification. Hook dispatch uses a small source fixture; it does
not establish that Ness's scripted Yoyo move itself matches stock behavior.
The 25/26 figure measures preparation, not completed clone fighters.
