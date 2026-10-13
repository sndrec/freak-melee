# Source Article debug owner and stage collision fixture

Actual source compiler/catalog, VM, Item actor/storage, attachment relations and
GObj pool execute. The native debug function/getter and both stage collision
bodies preserve actual source statements. Debug-global settings and map setup/
geometry leaves are controlled; map leaves supply collision position, floor
index and Bool result. Native result is discarded for the source Void operation.

`inherit_owner_debug_display()` uses current damage owner. A Fighter replaces
only low two debug bits; null/non-Fighter ORs the global mode into the existing
byte. Foreign Fighters are allowed. A nonnull stale owner fails before native
classifier access. `stage_collision()` and true select it_8026D9A0; false selects
it_8026DA08. Both preserve setup/map/position/floor ordering.

`item.attachment_active` compares current owner to
the revocable attachment relation. Clearing attachment leaves immutable Creator
unchanged but makes this property false. Initial missing/stale relations yield
false without dangling userdata reads; matching captured relations that change
before publication fail without outputs or replacement lookup.

Native mutation cases use an explicit completion observation after actual native
writes. Pure comparison mutation cases use a test guard observation after
matching relation capture. These functions have no authored callback in the
audited path; no such callback is claimed. Unrelated native families reject.
This fixture does not import DAT assets or establish game acceptance.

```powershell
python tests/native/script_article_motion/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-article-motion-review01
```

Use a fresh A: directory. All subprocesses are hidden with CREATE_NO_WINDOW.

The matrix compares all 256 initial debug bytes for null/non-Fighter/Fighter
owners and four global modes, with varied full Fighter flag bytes. Six direct
native collision comparisons cover both map paths, both native Bool outcomes
and the zeroarg form; real VM signals exercise every operation and the ownership
property. Negative cases include wrong Bool/arity/context/epoch, faulty or
mismatched source schema, stale initial damage owner, and source/owner retirement,
generation/data/kind changes, revocation and physical pool reuse after capture.

The grouped property extension executes `item.contact_fighter` (full U32 native
Fighter creation ID, retaining its bits when assigned to supported signed state),
`item.reflection_owner_transfer_disabled` (xDCC.b1), and
`attachment_owner.fighter.action_detaches_items` (x2070.x2071_b6) through actual
source compilation, receiver projection and VM storage writes. Null/non-Fighter
and stale initial contacts return zero; revoked attachment uses an explicit
exists guard and false local default. Foreign stock Fighter native fields are
read directly, without a source spec substitution. Captured target removal,
reuse, kind/userdata/class/native-generation changes, invalid caller epoch,
faulted Item actor and unchanged-output rejection are exercised. Pure-read
observers are test instrumentation, not callback or concurrency claims.

The common-property implementation is linked directly. Unselected class/root
scale operations abort explicitly; common-global data is absent. Package/assets
and raw native payload storage remain controlled as described above.

The reflector extension appends the actual catalog property and executes the
actual property dispatch, ArticleMotion read, compiler/VM and Item storage.
Seven native getter/direct/VM comparisons cover null, non-Fighter, registered
stock and source-owned Fighters, full U32 bits, physical retirement and an
already-reused raw pointer. `nativeFighterTarget_Reflector`, its Fighter registry
scan and `ftLib_80087460` are extracted from production without changed bodies.
Native reflection writes a registered Fighter contact. Deliberately unlinked
but physically live Fighters are not compared: the host getter scans Fighter
PLink, whereas the interpreter admits physical pool/class/native generation.
Neither read provides historical generation provenance for the raw native field.

Four cases execute the actual `Item_80269F14` body (renamed as a fixture entry)
and actual `processSourceCallback` with authored `on reflected`, without calling
a stock Item reflected callback. Both owner-transfer policy branches preserve
native team/metadata copying; the VM sees the transferred owner or preserved
owner and independently reads the reflector. A non-Fighter relation under the
preserve-owner policy reads zero. An actual VM divide-by-zero stops the entry
after owner transfer, before its enabled hitbox's damage continuation. The
damage, removal and NativeHoldable leaves abort if reached; the native damage
cap is controlled. Hitbox damage behavior and whole boomerang gameplay are
outside this fixture.

Fifteen reflector negative cases reject arity/epoch/generation, captured peer
kind/userdata/class/generation change, retirement/reuse and field replacement,
plus source retirement, pool reuse, generation change and fault. Pure guard
observations remain explicit test instrumentation, without an allocation,
callback or concurrency claim. No assertion simulates Item userdata replacement
inside an already-evaluating pure equality guard. Previous operation cases are
retained; the runner now selects unselected family rejection leaves from their
current filenames, removing its stale duplicated `.c.c` resource dependency.
Actual Item actors supply the creation-value read rather than its absent-actor
leaf, and unselected attachment release aborts here (its own proof is separate).
