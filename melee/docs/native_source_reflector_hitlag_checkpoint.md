# Interpreter reflector, hitlag observations and stage-line admission

This batch continues the retained Link and Young Link source migration after
`67954b7b5`. It adds reusable observations without moving authored decisions
back into fighter-specific C.

## Connected operations

- `item.reflector` reads the actual Item reflector relation. It neither
  substitutes the creator nor assumes the reflector is the current owner.
  Actual reflected entry transfers ownership before the authored callback;
  the keep-owner branch retains the distinct reflector. Null, non-Fighter and
  retired relations produce zero. Physical and current program/storage identity
  are checked, but a raw relation does not carry historical generation identity.
- `hitlag.active` reads the native active flag on physical Fighter receivers,
  including stock and independently authored fighters. It does not traverse
  linked hitlag pointers. Linked-ID queries and mutations retain their existing
  owned-source requirements. Link and Young Link now use the canonical
  `attachment_owner.hitlag.active` spelling.
- Article line-speed queries reject invalid authored line IDs before the native
  debug loop. Native `-1`, disabled and hidden line behavior is preserved;
  active geometry is checked only where the native routine dereferences it.
  Successful samples retain native values and unsuccessful queries retain their
  existing direct/VM output contracts.

The companion documentation now distinguishes already-supported alternate forms
from pending simultaneous companion activation. This batch changes no companion
runtime behavior. The remaining member mapping, constructor history and teardown
contracts are recorded in the [companion audit](native_source_companion_contract_audit.md).

## Validation and boundaries

The native client, content tool, source-assets target and selected integration
targets build; all sixteen selected CTests pass. No game, GPU or audio device
was opened. Root independently rehashed the following receipts and their pinned
inputs against the frozen production sources:

| Receipt | Scope | SHA256 |
|---|---|---|
| `A:/FreakMelee-test-archives/script-article-motion-reflector-oct10-final/receipt.json` | 6,568 checks, 3,078 retained native comparisons, seven reflector pairs, four actual reflected-entry VM cases, fifteen rejections | `306b9f76f027b563b92d4bbdc920eb0bc4c107b8085647a9a70e9fb7b1be8a32` |
| `A:/FreakMelee-test-archives/script-hitlag-observations-oct10-final/receipt.json` | 12,024 checks, 877 cases, 2,481 native comparisons, 918 VM calls including 576 receiver calls | `f7992e6ec45bc57c1d86273ddc1a6874523ba02bb9facb4132bcc6758c17e128` |
| `A:/FreakMelee-test-archives/script-item-resources-line-speed-oct10-final/receipt.json` | 2,592 checks, 256 native speed comparisons, sixteen refusals, twenty-four Item VM output cases | `129c2cc2cc47a69011265182dec9a833896af6129bee322de606ad8cd86c70ff` |
| `A:/FreakMelee-test-archives/source-reflector-hitlag-census-oct10-final/receipt.json` | Actual source-package preparation of ten retained full sources | `8f8a4358c4e0fd0bc6b1218fdcc1db7b8606929bb9cf9dc3327abd1aca81f11c` |

Reflector comparisons use native registered contacts; an intentionally unlinked
but physically live Fighter is outside that comparison. Hitlag tests use
controlled package lookups and payloads, not in-game hitlag timing. Line-speed
tests execute the actual native remapping kernel on controlled map tables. The
historical generated-C geometry fixture has pre-existing retired owner/path ABI
drift and is not a passing current interpreter suite.

Eight full sources still prepare: Mario, Dr. Mario, Peach, Game & Watch, Zelda,
Sheik, Popo and Nana. Link stops at line 3301 and Young Link at line 3373 on
`model_graphs.install`. Their retained Article bodies have advanced through the
reflector and hitlag gaps without being stripped. Preparation and these focused
native comparisons do not establish whole-fighter or paired gameplay acceptance.
Companion activation and compact whole-match save/load remain open; Regions/Pages
remains the broad rollback oracle.
