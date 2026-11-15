# Owner parameter reads and receiver fixture refresh

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The stale receiver integration fixture now builds and runs against the current
compiler, binding catalog, Execution/VM, receiver registry, and native GObj pool.
Its production catalog change only names argument descriptor fields: 242
three-field and eight two-field initializers plus the `ARG` macro. An independent
comparison against the preceding commit confirmed no other catalog changes.
All omitted flags still initialize to zero.

The fixture includes the current resolver inventory and native model-animation
demand census. It no longer links the unrelated Item-program import/preparation
unit. Unrelated bindings remain explicitly unavailable; new movement leaves
abort if reached. The collision-box helper comes from the native extraction,
removing the obsolete duplicate fixture definition.

## Parameter behavior

Source functions execute real receiver lowering and runtime reads with different
controlled values for the creator (2), current reflected owner (7), and executing
Item view (11):

- `creator.params.gain` stays on the captured creator after owner transfer.
- `owner.params.gain` reads the attached creator while the authored attachment
  contract holds and fails after that contract is broken.
- `current_owner.params.gain` follows the newly selected live damage owner.
- `program.attrs.gain` reads the executing view's immutable root table.
- Retiring a captured parameter-table identity rejects its read without
  publishing a result. Unknown parameter names and parameter assignments fail
  compilation.

This verifies the generic owner-parameter spelling used by Bowser's flame script
in the preceding damage checkpoint. It does not prove complete flame activation
or gameplay. The views and native motion/collision leaves are controlled; real
package import, reflection collision scheduling, and rendered behavior remain
outside this fixture.

## Verification

- Receiver suite: **4,444 checks / 17 Execution runs**.
  `A:/FreakMelee-test-archives/receiver-parameters-oct10-final/receipt.json`
  SHA-256 `216dd2d0f2cb0e62dbf0f104fd5b3a33fa2cf2ba9c8e13ee44dfcdc9811b854a`.
  **1,693 pins**, including headers, extraction scripts, compiler and README.
- Capture regression: **99,586 checks / 9,360 cases**.
  `A:/FreakMelee-test-archives/script-receiver-refresh-oct10-final/receipt.json`
  SHA-256 `109ee66de7b50210348d95b0a358698fa17ced344a0535a89963039e61b54ab3`.
  **1,810 pins**.
- Independent receipt rehash and catalog-only normalization evidence:
  `A:/FreakMelee-test-archives/receiver-parameters-independent-pins-oct10.json`.
- All seven configured Windows targets build. `native_fighter_script` CTest
  passes. `git diff --check` passes.

The earlier receiver build failures are preserved in their archive directories;
they are not runtime evidence. Full fighter activation and compact whole-match
rollback remain open. No installed assets changed and no game window opened.
