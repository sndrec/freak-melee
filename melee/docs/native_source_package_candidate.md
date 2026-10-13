# Source package candidate preparation

`native_source_package.c/.h` prepares a source-only candidate at its final stable
address. It consumes a data-only asset record, three DATs (fighter, animation and
costume), FMS source and an optional parameter text sidecar. It never consults a
donor Definition/code key, emits C, publishes global programs/assets, or changes
the current package/runtime/UI route. Source facts are decoded through the real
asset codec. Authoring may create those facts through Copy/Write; preparation
does not retain or call that donor.

The candidate owns one copied input allocation, a separate contiguous compile
bank and the asset importer's native descriptor arenas. Imported assets are built
directly in their final candidate fields, because internal parts/demo/data pointers
self-reference those fields. No structure copy follows import. The `address`
identity rejects moved candidates. Preparation requires a completely vacant,
zero-initialized candidate separate from the accepted candidate. It reads the
accepted bank only for compiler buffer independence checks. Every preparation
failure releases the attempted candidate back to vacant; accepted source, code,
parameters and imported assets remain unchanged.

Article metadata is mapped by the owned facts' declaration name and asset slot,
using the importer's decoded physical kind. Parameters copy actual root and
Article DAT attributes into immutable typed slots. The existing sidecar text
parser is reused as a data codec; names match exact indexed root or qualified
`Article.parameter` declarations. Unknown/ambiguous names, duplicate fields,
type mismatch, malformed text and unsigned/byte range violations reject. A
sidecar override replaces the matching imported supply or adds a declared default
parameter supply, without creating duplicate semantic IDs. No source rewrite or
live DAT alias is required.

The source bank bounds are explicit and provisional: source capacity 1 MiB
(including NUL), 16,384 syntax nodes, 65,536 IR nodes and arguments, 512 functions,
8,192 function parameters, 2,048 values, 512 locals, 131,072 instructions and 129
storage owners. All bank arrays occupy one load-transition allocation. The stats
report reserved bytes and active typed buffer bytes; the latter sums counts and
local high-water usage, excluding padding and unused reserved capacity. It also
reports instruction and register/depth limits. No candidate operation allocates
in a tick; the caller invokes preparation at a load transition.

Only the explicit Single input family is supported. Family is declared by the
caller, never guessed from asset roots or behavior keys. Non-Single inputs reject.
Source Workshop Items reject until an owned PNG/blueprint input and lifetime
contract exists. Required Article graphs/types and supported model-effect recipes
must validate; absent resources are not silently omitted. This does not establish
Rook, paired-fighter or arbitrary-resource closure. Shared engine resource loading
and actual activation remain the activation owner's responsibility.

CanActivate requires a stable prepared candidate, actual script/native scheduler
quiescence and the target's real publication preflight. It grants no publication
or engine-table mutation itself. Release checks all published kinds for the exact
bank, so a selected bank remains protected even before any actor attaches. It also
queries actual active/running Fighter, Item and Effect views by bank identity;
another live bank of the same kind does not substitute for that check. Explicit
transition Pin/Unpin leases cover external users that are not actor execution
views. Future activation must retain those leases for installed asset/descriptor
tables, previews/readers and **rollback history until its saved references drain**.
Replacing a program or losing its last actor does not by itself end those external
lifetimes. Release refuses outstanding leases and moved owners.

Root's actual-disc proof is
`A:/FreakMelee-test-archives/source-package-bounds-root/receipt.json`:
1,536 checks / 25 cases. It prepares the complete Mario source with 77 function
bodies, two Item programs and one model effect, 31 root plus four Fireball imports,
and two exact sidecar overrides, using copied real DAT/animation/costume assets.
It verifies failure isolation, stable self-references, leases, moved-owner rejection
and selected-bank release rejection. Added asset checks reject kind reads beyond
the actual DAT attribute extent and across raw-byte schema ranges. Its compile bank reserves 14,175,808 bytes;
active buffers use 276,722 bytes with 1,741 instructions and 19 maximum call
registers. These are preparation/component results, not full native gameplay,
rendering, package-format activation or rollback-performance acceptance.
