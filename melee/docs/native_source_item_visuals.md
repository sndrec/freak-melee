# Source item visuals

`NativeSourceItemVisuals` is embedded in the candidate source package at its
final address. Loading calls `nativeSourceItemVisuals_Prepare` after item recipes
and their pinned resource bytes are prepared. It validates the source programs,
PNG framing and CRCs, then decodes pixels with the real image decoder. Only a
complete successful preparation installs immutable `NativeScriptItemProgram.visual`
pointers. Failed staging frees candidate pixels without editing accepted
resources or binding a partially decoded set.

The program registry can copy/rebind its internal recipes and state records.
Visual pointers remain external pointers to the stable resource owner. Each
visual records its exact compile bank, program identity, source declaration
owner, encoded PNG identity and billboard size. Actor/program/package lifetime
checks pin this resource through use; there is no global fighter/index texture
registry.

Declarations in one bank have separate program identities. Preparation accepts
their shared bank and preserves each declaration identity in its own record;
it does not require unrelated item declarations to share an identity.

`nativeHoldable_Draw` obtains the actual item Spec and uses that exact visual.
It performs no decoding or allocation. PNG billboards draw in the translucent
pass using their prepared dimensions, alpha texture and authored world size.
If a declared PNG visual is missing or mismatched, it does not draw another
recipe's texture or a model substitute. A recipe with no PNG instead draws its
own prepared Article model through the common item renderer, after checking
the instance's Article identity. That is owned model data, not donor behavior.

`nativeSourceItemVisuals_Release` requires an idle VM, no executing/deferred GObj
work or active draw, an unselected bank and no exact Fighter/Item/Effect bank
references. Rejected release preserves the entire owner. Package retirement
also enforces its own leases and retained-history checks before releasing
visuals and the rest of the candidate.

PNG inputs share the canonical resource codec's bounds: 262144 encoded bytes
and dimensions at most 1024. Decoded texture storage can be larger than encoded
data; allocation occurs only at the explicit loading transition. Structural
PNG validity and successful platform decoding are separate checks. Platform
decoder tolerance does not establish strict compressed-stream integrity.

`tests/native/source_item_visual` verifies actual decoding/tiled pixels,
transactional failure, pointer publication and draw/lifetime contracts with
controlled registry and graphics leaves. Full package and real presentation
acceptance remain separate engine tests.
