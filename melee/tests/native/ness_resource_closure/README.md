# Ness resource closure: unregistered source fixture

This source-only checkpoint composes all four resource handles and Bat/YoYo
construction helpers with the accepted Thunder candidate. `candidate.fms` is
**not registered** and must never be copied into the default Ness template.
YoYo initialization and smash phases remain incomplete.

Run from the repository root on Windows:

```powershell
python tests/native/ness_resource_closure/run.py `
    --snapshot .local/baseline30 `
    --output .local/ness-resource-closure-source30
```

The snapshot supplies `freeze.json`, `sdk/`, `bin/native_content_tool.exe` and
`compiler/clang.exe`. The runner verifies SDK/tool/compiler hashes, checks
original source hashes against preserved `*_original_inputs.json`, extracts
unchanged function bodies, generates callbacks, and directly compiles two console
executables with frozen includes, vcvars14.44 and `-ffp-contract=off`. It requires
Visual Studio's installed C toolchain, located through vswhere. No ISO, game,
global build, `script-build`, package or live source include paths are used.
Generated C, originals, commands, executables, logs and a fresh hash receipt go
only to `--output`, which cannot be inside this namespace or snapshot.

The fixtures are ordinary C sources with explicit engine boundaries; the runner
does not rewrite expected bodies to match authored results. Preserved receipts
also record the historical accepted Thunder hash; that existing external source
is not required to execute this self-contained namespace.

## Checked source scope

The lifecycle executable passes **67,270 checks**:

- 1,408 cleanup cases: all16 four-resource presence masks,11 Thunder motions,
  reflection on/off and four pause/x7 combinations. Full high-bit item identities
  are retained through generated locals and signals.
- Exact YoYo -> Flash -> Thunder -> Bat cleanup order, nested YoYo resume before
  handle clear, outer resume after recipient clearing,20-node tail-to-head
  release, owners/bound flags, damage/death callbacks, GFX and root reset.
- Original death's group0 hide/reset without resource removal, plus owned load
  sentinel initialization after attribute copy.
- Original fighter constructor/publication decisions for normal Bat and YoYo:
  success/failure, all four preexisting damage/death2 callback masks, auxiliary
  publication/clear and hitlag callbacks installed even on failed YoYo creation.

The Bat executable passes **122 checks**. It extracts both actual Bat constructors,
their actual no-initial-collision SpawnItem initializer and pickup callback.
It checks flattened current/previous positions, parent pair, zero velocity,
collision flag, mode command fields, normal-only1200 lifetime then attachment,
and all16 pickup command masks followed by animation advance.

## Explicit engine boundaries

`lifecycle_compare.c` uses controlled allocation, constructor, signal dispatch,
recipient export, visibility, root/effect and resource-free boundaries. Signal
dispatch selects actual generated callbacks by hash and validates fixture IDs;
recipient export invokes the actual generated export. This is a callback/order
comparison, **not a new actual-backend author/kind/logic/depth acceptance claim**.
Actual generic resolver/lifetime tests remain separate. Original pause/resume
functions themselves are extracted rather than replaced with duplicate flag code.

`bat_compare.c` mocks the Item allocation and attachment/state/animation advance
boundaries. Original constructors still build and expose their actual descriptor;
generated `on created` and picked-up callbacks execute directly. Failed engine
allocation intentionally skips generated creation; engine constructor lifetime
continuations have separate generic coverage. Bat per-frame model animation is
compiled but not accepted by this matrix.

## Pending gates

YoYo original `it_802A24D0` assigns `CollData.cur_pos = link.pos` before
`mpColl_80041EE4`, which seeds last/prev/x28 history from that position. Existing
resource-node creation initializes collision at zero before authored position
writes. Current `collision.position` and `collision.previous` expose only cur/
last, so no offset workaround is used. The draft is pending a generic optional
initial XYZ input to the substantive node constructor before collision init.

Four YoYo solvers and actual string/tip model-graph initialization remain open.
Failure cleanup deliberately proposes deterministic atomic release instead of
original uninitialized-prev_link/UAF behavior; no successful YoYo init parity is
claimed. Seven smash motions341..347, charge stats, capsule reset/rearm, local
box sweep, up/down smash entry hooks and common presentation hooks remain open.
Presentation Bat uses common `item_gobj`, not the smash handle, and includes
Sleep/DeadUpFall plus DeadUpStarIce's0.8 attribute-scale factor. Original normal
Bat/YoYo use raw joint42; presentation resolves RThumbNb (currently mapped42 in
owned Ness rig), distinct from semantic R2ndNa (mapped35).

The combined candidate's historical fresh normal frozen30 import had157 named
parameters,43 fighter fields and11 articles, and its generated C compiled with
the explicit frozen SDK. This runner reproduces the bounded two-family source
matrix only; it does not establish package or gameplay acceptance.
