# Ordered Peach resource cleanup: unregistered source candidate

The SDK38 console fixture passes **3,420 actual/generated pairs, 33 separate
safety cases, and 87,163 checks**. It compares ordered Parasol → Toad → Turnip
cleanup and saved-held-item restoration. This candidate is not installed into
the Peach template or lifecycle table, and establishes no gameplay or complete
Peach independence claim.

Run from the repository root:

```powershell
python tests/native/peach_resource_cleanup/run.py --root . --baseline .local/baseline38 --output .local/peach-resource-cleanup38
```

The runner uses the frozen content tool and compiler, extracts 28 unchanged
production function bodies plus one exact held-item statement slice, links the
real `native_item_owner`, callback registry and recipient-export dispatcher, and
records the commands and SHA-256 inputs in `result.json`. Function instrumentation
observes the entries of real stock and generated helpers; it neither wraps nor
rewrites their bodies. Every ordered event and projected final field is compared.

## Healthy source comparisons

The 1,440 cleanup pairs cross eight resource-presence combinations, three saved
references (none, Turnip, ordinary item), five current-held references, three
saved-item current owners (null, source, foreign), and four full-width IDs
including zero and high-bit values. An additional 1,920 pairs change Parasol
current ownership to a different live
recipient in the same package. The original creator has either no saved item or
a saved Turnip; the recipient independently has either no saved item or a saved
ordinary item. This crosses the same resource-presence/held references, null/
source/foreign ordinary-item ownership and full-width IDs. The recipient held
slot, saved/Parasol references and callback flags are compared independently.
Both false and true restoration returns are observed through the unchanged
stock owner-clearing branch and compared with the typed generated recipient call.
The remaining 60 pairs compare the precise
producer save → hide → pause → drop statement slice. The ordinary item's original
creator is foreign; restoring it preserves both that creator record and its
current owner. Binding changes only the fighter slot, without attachment.

Repeated Parasol restoration and Toad notifications are retained. Article removal
runs the actual destruction callback and actual removal body; destruction can
clear current ownership before removal continues. Turnip cleanup still requires
the saved Turnip to be the currently held item. Projections include all resource
references, held slot, callback presence, live registry membership, joint flags,
the full item flags word, current ownership, and original-creator identity.

## Separate safety checks

These do not compare unsafe original pointer continuations with the script.
They cover source removal/recycling/kind/Definition changes during drop; removed
or recycled saved items; source/item/Parasol mutation during pickup; stale saved
handles; missing model root; and held/auxiliary binding with each saved current
owner and full-width ID. Recycled items receive a distinct live ID rather than an
ID already held by another object.

The actual SDK38 `ftpickupitem_80094818` body stops its stats tail after source
removal or recycling. Earlier SDK35 healthy comparisons matched, but SDK35's
internal pickup tail lacked that guard. Its scratch evidence is historical and
is not the final safety acceptance. SDK37 subsequently passed the healthy
source-owner subset and repaired safety boundaries; SDK38 extends that evidence
with typed recipient results. Missing-model restoration binds and resumes
the live saved item, clears the saved reference, and skips pickup notification.

## Controlled boundaries and remaining gaps

- Parasol restoration is now `export func restore_held() -> bool`, and its
  article callbacks invoke `targets.call_bool(item.current_owner, "restore_held")`.
  There is no `restore_result` transport field or creator-state reply channel.
  Actual missing/present saved references determine the recipient result. This
  covers source and changed live holders in the same registered package;
  arbitrary unregistered holders and unsafe original stale pointers are excluded.
- The producer comparison is only the unchanged save/hide/pause/drop block in
  `ftPe_SpecialHi_8011D424`. Request gates, bone/world position, allocation,
  attachment and constructor tail are excluded. Stock `it_802BDA64` calls
  `Item_AttachToParent` (AB54 followed by debug notification) without auxiliary
  binding. Existing generic attachment alone does not reproduce that entire
  constructor trace. No debug event is fabricated in this harness.
- Actual show/hide and pause/resume bodies run, with controlled HSD subtree flag
  operations. Actual pickup/drop bodies run with controlled fighter extension
  callbacks and statistics sinks. Removal housekeeping, allocation, effects,
  dynamic-bone destruction and unrelated item-switch work are controlled;
  allocation, attachment and scale stubs assert if this fixture reaches them.
- Definition lookup and logic installation use the generated module in a small
  fixed live registry. The current-owner query uses real registry snapshots;
  unrelated special pickup branches are disabled. Full native scene/model
  traversal, external callbacks, constructor failures and Kirby are outside this
  comparison.

The accepted constructor and complete Turnip phase fixtures are unchanged.
Evidence: `.local/peach-resource-cleanup38/result.json` and `run.log`; historical
SDK37 receipt remains `.local/peach-resource-cleanup37/result.json`.
