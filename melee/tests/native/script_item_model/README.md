This console fixture compiles every probe body through the direct source frontend,
prepares ordinal joint selectors against a controlled owned `HSD_Joint` graph,
and executes actual bytecode/VM/catalog/model bindings on an Item in the real
fixed GObj pool and native callback registry. Actor program/storage views are
controlled; the binding validates them substantively. No assets or scenes load.

The native joint read/write implementation and extracted HSD subtree flag and
class-ancestry bodies execute directly. Tests compare Euler rotation, scale,
visibility, signed scalar values and nonfinite policy with those primitives.
Uniform root scale preserves the distinct native nonfinite contract and does
not change Item.scl. Animation leaves are controlled and verify exact stage
order plus generation retirement after each of the four stages.

The selector table uses an explicit destination buffer. Out-of-range model index,
nonliteral/out-of-range dynamic index, malformed/oversized paths, unsupported projections, readonly
writes and type mismatches reject source compilation without changing the
accepted bank. Missing descriptor paths remain valid nullable selectors;
aggregate capacity exhaustion preserves the prepared table. Secondary Scene
operations explicitly abort in this primary-only fixture; connected secondary
model acceptance belongs to `source_script_item_models`. Named animation
resources remain unsupported; this receipt does not claim complete Zelda.

The final fixture rejects foreign layout/parameter pointers despite matching
contents, wrong owner, retired storage, absent selector storage and a different
compile bank. JObj subclasses are accepted through the actual HSD ancestry
check; an unrelated undersized class is rejected before a native joint read.
Missing runtime joint paths retain the native default/no-op semantics after
successful load-time descriptor validation.

Initial selector-only receipt: `A:/FreakMelee-test-archives/script-item-model-freeze/receipt.json`.
It records 194 checks, 16 native comparisons, nine prepared selectors and four
controlled animation-stage retirement checks. The DATA advancement extension
has its own fresh receipt; it does not retroactively extend that earlier run.
No shared build or game was launched by this fixture.

The advancement extension executes the original `Item_802694CC` reference body
and the actual `it_802799E4` DATA interpreter, `Command_Execute`, damage/radius
command handlers and hitbox scaling/damage helpers. Comparisons cover all four
hitbox slots, enabled/disabled hitboxes and owner/no-owner damage paths. HSD
animation and owner damage adjustment leaves are controlled; this is native
DATA command parity and sequencing, not animation asset/math parity.

`item.advance_animation()` preserves native order: animate the primary root,
copy its current frame, execute DATA commands. Guards run between those stages
and before output publication. `item.advance_model_animation()` animates only
the primary root. The native interpreter is a synchronous native DATA stage;
it does not dispatch authored FMS callbacks. Retirement injected into a native
damage-adjustment leaf is detected when that stage returns. Source tests also
execute both operations through the actual catalogue and VM.

Imported DATA assumptions remain explicit: `itCommandData_Import` validates
opcode widths and branch offsets and transcodes command fields. The original
implementation did not validate hitbox indices, model bone indices or loop-stack depth.
The shared command importer now validates those reached operands and typed stack
paths at loading; its separate corpus/topology fixture provides that evidence.
This model fixture is not a claim that every malformed DATA command or effect
dispatch is safe. Hitbox creation with nonzero bone index
uses the actual dynamic bone table; this fixture's damage/radius commands need
no such table and does not claim imported bone-table acceptance.

Final advancement receipt:
`A:/FreakMelee-test-archives/script-item-advancement-freeze/receipt.json`.
It includes the selector checks plus 16 native DATA advancement comparisons,
both typed source methods, model-only state preservation and all three
advancement retirement stages. The earlier selector-only receipt remains
historical evidence for its original scope.

The appearance extension executes `item.set_appearance(I32, F32)` through the
actual frontend, bytecode, VM, catalogue and model binding. Twenty comparisons
against the extracted original `it_80273670` cover both descriptors, a
present/absent source joint, and exact positive/negative/signed-zero/nonfinite
frame payloads. The real `lb_8000B804` transform reset runs; joint,
material and shape animation leaves are controlled and check their exact
arguments and six-stage order. These comparisons cover the complete Item
payload and primary joint graph, including selected descriptor, cleared DATA
pointer, unchanged native motion/script revision and unchanged recorded frame.

Negative cases cover invalid indices without mutation, missing table/storage,
malformed direct arguments, native generation retirement, logical storage
retirement, replaced model/userdata, nested state replacement, changed descriptor
table/program identity, and same-slot GObj recycling after every animation
stage. Nested state setup keeps its new DATA pointer when the outer bake stops.
Physical and logical retirement also execute through the real VM and preserve
the caller's output instead of publishing its later return value.
This fixture retains controlled actor/spec/storage views and HSD animation
evaluation; it does not prove imported animation math, rendering or gameplay.
The native routine assumes a valid descriptor index; the source oracle and
direct operation both admit only the owned variable-length table range.

Final appearance receipt:
`A:/FreakMelee-test-archives/script-item-appearance-final/receipt.json`.
Run with `python tests/native/script_item_model/run.py --compiler
.local/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/NEW`.
The runner requires a fresh A: directory, hides each bounded console process,
records commands and fingerprints all input sources, headers and the compiler.


The Oct 9 spin/scale/projection extension uses direct native `it_80274740`,
`it_8026BAE8`, `it_80274484`, and `it_80274594` bodies, plus their actual uniform
scale, hitcaps, grab-range and collision-range helpers. Thirty-four comparisons
cover descriptor child depths 0..3, all four native axis values, and negative,
signed-zero, zero, ordinary and nonfinite F32 scale inputs. Full Item/JObj
payload comparisons preserve the difference between attribute/root scale and
instance scale refresh. Inheritance reads the current foreign Fighter damage
owner's `x34_scale.y`, rather than immutable creator or attachment metadata.
A real pool allocation and registry entry identify that owner.

Spin reset admits only the exact owned descriptor and a present Euler joint;
missing depth, missing descriptor and quaternion joints reject before the native
setter's assertions or spin write. Attribute scaling requires the exact owned
common attribute pointer. Inheritance rejects absent, non-Fighter, unregistered
or stale current owners; it invents no scale-one default. Source/model/storage
and owner identity changes withhold output. These four native operations have
no authored callbacks: fixture invalidation is explicitly injected after each
complete extracted native body, rather than claiming internal reentry behavior.
The existing actor/spec/storage views remain controlled; no complete actor
scheduler acceptance is claimed for this fixture.

Translation channels 0..2 and single-node `local_visible` channel15 execute the
same prepared primary selectors and actual joint primitives. Source helpers
execute through compiler/bytecode/VM/catalog. Translation uses the native finite
write/no-op policy and missing joint zero defaults; recursive `visible` retains
channel10 semantics, while local visibility changes neither child nor sibling.
Missing local visibility reads false. Tests retain malformed path/type/access
and failure-isolation checks, using genuinely invalid `translation.w` instead
of the newly supported X channel.

The final grouped matrix records 1,928 checks, 95 native comparisons total,
34 spin/scale pairs and 35 added scale rejection/continuation cases.

Final extension receipt:
`A:/FreakMelee-test-archives/script-item-model-scale-final2/receipt.json`.
Strict compiler exclusions include existing native unused padding variables,
Dolphin array parameter spelling and partial test fixture initializers.
No secondary graphs, imported asset animation math, renderer or game were run.


Whole-model controls have a separate receipt:
`A:/FreakMelee-test-archives/script-item-model-controls-final2/receipt.json`.
The grouped run records 2,488 checks and 151 native comparisons, including 56
new traversal/rate comparisons and 20 continuation/rejection cases. Show/hide
execute the actual Item and HSD flag helpers across root, child, child sibling,
instance and root-following-sibling boundaries. The primary model's own next
sibling is not traversed; instance children are skipped.

Animation-rate coverage now executes real `lb_8000BA0C`, `lb_8000B9D8`,
`HSD_JObjWalkTree/0` and `HSD_AObjSetRate`, replacing the former controlled rate
leaf. Parent links and AObj records are valid explicit fixture graphs. The native
callback intentionally skips the primary root's AObj; descendant joint AObjs
change, null AObjs do nothing, and material/texture/shape AObjs stay unchanged.
Negative, signed-zero, zero, ordinary and nonfinite F32 inputs remain unmodified.
The Item animation-speed field is unchanged. Existing appearance stage observers
remain controlled post-native observations, and previous matrices still pass.

The three methods execute through source compiler, catalog, bytecode and VM.
Output retention and later-source-statement suppression are checked for physical
and logical retirement. Identity changes are injected only after each complete
native traversal; no native callback or natural field-read reentry is invented.
This retains the controlled actor/view/asset boundaries described above, rather
than claiming scene or game acceptance.

Primary world-position queries extend this same compiler/VM fixture. Three
prepared call recipes exercise root, child and missing paths; three Float
outputs publish together, including a missing primary root's zero result.
The complete native `lb_8000B1CC` executes. Matrix setup is a controlled leaf,
with explicit post-matrix identity mutations covering source, storage, graph,
selected class, package and layout changes. Native coordinate bit patterns are
compared for finite, signed-zero, infinite and NaN inputs. This does not prove
the renderer's matrix construction. Duplicate/non-writable/wrong-type outputs,
out-of-range model selection and malformed paths are compiler rejections.

The Oct 10 dynamic-joint/matrix extension prepares the distinct dynamic table
index directly, without deriving a literal child path. The constructor evidence
is `Item_802680CC` -> `HSD_JObjLoadJoint`/`JObjLoad`, which stores the immutable
descriptor address in each JObj.id, followed by `Item_802682F0`, which fills the
Item's dynamic table in native preorder. The backend retains the exact table
pointer and selected entry, then checks a nonnull entry against that owned
descriptor/live preorder before dereferencing the selected pointer. The table
allocation itself relies on the existing native Item allocation invariant; this
fixture does not establish arbitrary pointer or imported factory admission.

Six hundred thirty comparisons execute the extracted native
`nativeArticleModel_ReadDynamic`/`WriteDynamic` and the actual joint primitives.
They cover all scalar, quaternion, existence and visibility channels; Euler and
quaternion flags; present, null-table and null-entry cases; signed zero,
nonfinite and ordinary values; and full joint payload parity after writes.
The actual compiler and VM execute dynamic writes and the index99 missing-entry
default. Seven admission cases reject mismatched literal recipes, wrong argument
type, a foreign selected pointer, wrong ordinal/id/class and stale generation.

Twenty-four matrix comparisons use the extracted native
`nativeArticleModel_ReadMatrixRotation` and real `HSD_MtxGetRotation`/`calcVal`.
Root, child and missing paths cover rotations, zero/degenerate/nonfinite matrices,
missing roots and missing runtime paths. Three Float destinations publish the
native result; missing selectors publish zero. Dirty flags and complete joint
payloads stay unchanged, and the existing matrix-setup observation count stays
unchanged. These reads intentionally do not set up a matrix or read local Euler
fields. Root/child/missing source functions also execute through compiler,
bytecode, catalogue and VM. Secondary graph construction remains outside this
primary fixture, with its native Scene boundaries explicitly unavailable.

Twelve added compiler negatives cover dynamic literal/type/index/access rules
and matrix model/path/output contracts. Actor/spec/storage views, table memory
and graph construction remain controlled as above. No natural native callback
or per-tick retirement is invented for these synchronous field/math operations;
the production source guards run before and after their native primitives.
This proof does not claim imported Bomb controller, asset loading, renderer or
gameplay acceptance. Generated native bodies and every source/header/compiler
input are fingerprinted and rechecked before receipt publication.
