# Fountain reflection ownership

The original reflection userdata now resides in actual `stage_hazard_state.c`
storage with the parameter binding and camera GObj owner. Source constructors
bind the fixed record directly and use its actual userdata retirement callback;
that callback clears the owner instead of freeing static storage. Source drawing,
camera mirroring, framebuffer copy and matrix calculations remain unchanged.

The stage-hazard component captures the reflection matrix/image binding and
owning GObj generation. It also claims the reflection CObj's retained class span
and admitted type. The class owner retains that camera's payload; this component
does not copy it twice. Image descriptor contents/address are leased unchanged
after source initialization. Required camera roles, userdata/destructor identity,
class type and image binding must agree before local Load writes.

The component is now 5,768 bytes, forty bytes larger for its camera claim; there
are still 32 census components. No snapshot callback, camera update, rendering or
allocation occurs. The owner lease is outside simulation storage.

`HSD_ImageDescCopyFromEFB` reads the descriptor and calls `GXCopyTex`. Aurora's
native GX path publishes the destination key and resolves the framebuffer into
GPU textures, retaining them in `g_gxState.copyTextures`. It does not write those
pixels into the native destination RAM. Descriptor contents are initialized once
by `lb_800121FC`. The game does directly consume `texture_matrix` when drawing
the reflected floor, so that matrix is snapshot state. Normal reflection drawing
remains enabled to regenerate GPU pixels; no drawing-disabled replay path is
introduced. Full aggregate replay must still verify that regeneration/order.

Evidence under `A:/FreakMelee-test-archives/`:

- `fountain-oct11-owner-b`: 58 added checks/six malformed candidates, actual
  storage/serializer/retirement callback, matrix restore/repeated updates,
  camera span/type and object resurrection, frozen descriptor guards, wrong
  userdata callback refusal without partial matrix writes, overlaps/activity/
  retirement and untouched destination marker bytes. Camera/image/root setup and
  matrix updates are controlled; this is not a renderer or full-match test.
- `fountain-oct11-gm-a`: connected production control/player/camera fixture
  passes. Four native build targets and focused GObj-consumer CTest pass.
- `fountain-oct11-stages-a`: Fountain of Dreams, Rainbow Cruise and Final
  Destination old/current rendered CPU matches pass at 4,800 samples each,
  with exact populated traces and forty passing stage-private captures and
  validations per scene. Fountain's sampled component mean Save is 3.205 us,
  Validate 0.562 us. No aggregate Load or full timing bound is exercised.
- `fountain-oct11-eight-a`: stock and independent interpreted eight-actor
  scenarios pass all 32 components at ten boundaries each. Previous/current
  and diagnostic-on/off traces match exactly.
- `fountain-oct11-independent-verification.json`: final frozen/live hashes,
  41 source bodies with the explicit constructor/storage substitutions and
  compiled actual `.mstate$m` versus host `.data` sections verified. Rendering
  callback bodies and matrix calculations are unchanged.

Other world/HSD/HUD/effect owners and candidate-wide floor-Ground/camera/graph
agreement remain open, alongside complete aggregate restore and timing. The
broad page oracle remains in production.
