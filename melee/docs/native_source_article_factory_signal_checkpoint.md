# Direct positional Article factories and signals

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

This slice extends the ordinary bytecode interpreter, not its test-only generated
C oracle. No game, installed character package, or public draft was changed.

## Factories

`articles.create_at_position(DeclaredArticle,x,y,z,facing,origin_at_owner)` and
the twelve-argument `articles.create_with_velocity` lower a literal Article
declaration to its source owner. Built-in `It_Kind_*` names retain ordinary
constant resolution; runtime creation uses the encoded declaration identity,
not that argument's physical ItemKind value. Recipient-qualified factories,
computed kinds and undeclared numeric kinds cannot bypass owned admission.

The Bool selects current damage-owner item center, not grounding. Supplied
previous position is retained. Velocity precedes native model construction;
collision groups preserve all32 bits. Its final explicit Bool sets initial
collision in the native spawn descriptor; it does not imply grounded spawning.
Native constructors select their normal
airborne/default fields. Creation value is installed before Created. No forced
state, animation advance, launch callback, appearance change or stock authored
handler is substituted. Normal allocation/selector absence returns -1. Retired
relationships, callback faults and a removed newborn withhold output while
preserving any native side effects already completed. Published handles must
fit the declared signed Int width.

The native positional backend also supports Item callers. Original creator,
mutable damage owner and creating Item are separate retained relations. The
actual common constructor now captures a source parent's original creator
before Attach/Spawned, so reflection cannot incorrectly make its new damage
owner the child's original creator. Genuine creator absence remains valid;
stale non-NULL creator or damage-owner references reject creation. Public
Item-source creation bindings remain open.

## Signals

Fighter target delivery is `articles.signal(handle,DeclaredArticle,"name",value)`;
Article self delivery remains `item.signal("name",value)`. The load-time resolver
checks literal declaration/name, exact owned callback ABI (Void, one F32), and
encodes its code ID. Same names on distinct Articles are unambiguous. Function
ID0 is valid. Literal Text spans are marshalled and bounded at runtime; their
contents are never parsed or hashed there. Source-only admission prevents
foreign helper/recipient banks from borrowing these encoded identities.

Static signal/helper edges contribute to register/depth bounds and recursive
chains are rejected. Runtime delivery uses the prepared source Item signal
table and real actor/VM; no authored C callback or fake Definition is needed.
Missing/nonmatching targets return false from a live caller. A entered handler
may delete its recipient and still report delivery. Caller retirement or a
handler fault stops execution and preserves outputs. Targeted nonfinite values
retain native false delivery; self calls preserve F32 NaN/Inf as the old direct
local call did. Reflected damage ownership does not change source-bank identity.
Article-to-other-Article target delivery remains unconnected.

## Evidence and limits

Receipts are under `A:/FreakMelee-test-archives/`:

- `script-signal-catalog-oct08-root-final/receipt.json`:71 checks, actual compiler,
  graph/VM, function0, duplicate names across owners, malformed calls and
  accepted-bank preservation. Native signal capture leaves explicitly reject.
- `script-article-signals-oct08-root-final/receipt.json`:99 checks, real source
  program preparation/publication, Fighter/Item actors, nested VM delivery,
  foreign Fighter damage ownership, full32-bit handles and malformed spans,
  self NaN/Inf, target faults, caller retirement, deletion and physical reuse.
  Asset metadata and position/engine leaves are controlled; no DAT constructor
  or gameplay/render acceptance is inferred from this signal fixture.
- `script-positional-bindings-oct08-root-final/receipt.json`:153 checks/19cases,
  real source-to-VM-to-binding-to-native constructors/actors/pools, including
  both positional forms, velocity and full collision-group publication.
- `script-positional-article-spawn-final/receipt.json`:10566 checks/26cases,
  real constructors, source callbacks, creator capture, storage/GObj reuse,
  reflection and retirement.1024 reuse cycles and1048 native frees.
- `source-signal-contract-migration-oct08/receipt.json`:271 calls migrated in54
  tracked examples/test files. Representative test-only C oracle compiler
  contracts pass both Fighter/Article routes, reject the old three-argument
  target form and require the actual kind-reference host operation. That oracle
  samples handle/value once and checks its exact declared kind before delivery.
  The larger historical oracle gameplay matrices were not rerun in this slice.

Assets, model/geometry, immutable selection and some physics/platform leaves
are controlled in constructor fixtures; this does not prove rendering,
gameplay or a complete source character. The unified client/content-tool build
and36 selected CTests pass. Fresh actual private-owned-data source checks now
stop at Zelda's Article reference at196 and Sheik's Chain metadata at1543.
The next closure is retained Article references/state/translation, ownership
changes and child creation, then Chain collection/model/collision/matrix
ownership. Paired full gameplay and explicit rollback state remain open.
