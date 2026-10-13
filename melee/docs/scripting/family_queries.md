# Family queries

`family.member_id(index)` returns the native Fighter spawn ID as an `int`,
preserving all 32 handle bits. The index must be integral. Negative indices and
indices outside the two-member capacity return zero without narrowing wide
integers into a valid index. Missing members return zero. A malformed value or
retired/faulted source rejects the operation and withholds its result.

The operation requires the caller's selected source package, program, storage,
motion layout and physical scene identity. Nonzero results retain the actual
physical Fighter until publication. The binding calls the runtime member query;
it does not interpret Player role indices as immutable package member indices.

Member indices identify the active package's immutable members, rather than
Player roles. The runtime checks both Player roles for the exact registered
kind, family, package and published program. Either member can query both
members after an alternate-form role swap; a dormant alternate actor remains a
member. The caller must itself be a published, live interpreted member of that
Player's selected character. An unpublished caller, absent member or invalid
family/Player relationship produces zero.

Native CPU preparation uses a separate history hint. Only simultaneous member
one selects already-published member zero in its own Player and family. The
constructing caller need not have a published Player role; its native identity
and selected package/program are retained, with actor storage checked whenever
present. Primary, Single, AlternateForm and unavailable donors produce zero.
Normal `Fighter_Create` attaches the interpreter before CPU preparation, then
Player publishes the returned actor. The native initializer sends the hint as
signed Int bits to `cpu_prepare` and uses that callback's returned handle when
seeding its 30 pose records. Missing donors retain the native zero-position,
positive-facing fallback.

Simultaneous-companion preparation and activation now use the same transaction
as other source families, with independently owned assets, programs and CPU
profiles. The [activation checkpoint](../native_source_companion_activation_checkpoint.md)
records full Popo/Nana publication and eight live actors after 360 match samples.
The lookup and hint alone do not establish linked-special, Sleep teardown or
whole-character behavior; those require their own connected checks.

`family.retire_companion_on_sleep(enabled)` accepts a strict Bool and sets the
shared Fighter Sleep policy flag `x2222_b5`. It changes no other field and calls
no callback. The existing Sleep path performs normal visibility/camera setup,
retires player entity one if configured and present, then updates match status.
This setting neither creates a companion nor establishes family membership.
