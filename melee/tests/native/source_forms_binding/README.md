# Generic source form activation binding

`forms.activate_partner()` is a zero-argument Fighter-only Void intrinsic with
the native invalidation flag. The catalog emits its typed semantic ID directly.
The substantive binding validates the running caller's exact SourceFighterLayout,
calls `ftCommon_ActivatePartner(source, NULL)` and publishes Void only after the
common framework succeeds and the caller's lifetime/layout remains valid.

The framework owns alternate-pair membership, dormant destination checks and
FormActivated dispatch. CanActivateForm is a **destination** preflight: its
running==0 requirement must not gate the running source script. Both members'
FormActivated preparation requirement belongs to package/program validation.
No native callback thunk, per-fighter policy or allocation is added here.

Ordinary dormancy preserves source identity/view and permits return. Framework
false, logical/physical retirement, generation change or a fault leaves the
caller's result untouched. Failure is an error, never authored Void success.

Run `run.py --compiler <MSVC-target clang.exe> --output <fresh A: folder>`.
The receipt at
`A:/FreakMelee-test-archives/source-forms-binding-oct08/receipt.json` passes
**57 checks** against the actual catalog, semantic/bytecode compiler, top-level
Invoke, forms binding and exact extracted source ownership/lifetime guards.
Selected metadata/View and the transfer leaf are explicitly controlled. The
leaf asserts NULL finish and destination-only CanActivateForm; cases cover pair,
destination/framework rejection, retirement/fault and surviving dormancy.

This does not claim the actual native transfer implementation, real alternate
package or gameplay. Those comparisons and runtime gates belong to the other
workers. Unrelated runtime families are unavailable. Old catalog-only fixtures
link `tests/native/script_bindings_unavailable_forms.c`, a rejecting test-only
leaf; source form-transfer tests link the substantive forms unit instead.

The prior Rook census, ledge and jump-catalog standalone runners now include
the absent forms leaf. Only the jump catalog was repeated to verify that link
correction: `A:/FreakMelee-test-archives/source-jump-catalog-forms-dependency-oct08/receipt.json`
passes its original 39 semantic checks. This is separate from the new 57-check
forms contract receipt; older suites were not recounted as form evidence.
