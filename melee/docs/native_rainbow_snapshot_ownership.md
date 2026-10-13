# Rainbow Cruise moving and disappearing platform ownership

The source's one map-1 Ground owns seventeen moving-platform records and twenty
disappearing-platform records. Those arrays now reside directly in
`stage_hazard_state.c`, with the parameter binding and owning GObj. Both source
constructors bind their Ground pointers to the actual arrays; there is no live
heap mirror or replacement platform state machine. Static assertions keep the
array counts aligned with the source descriptor tables. The parameter structure
moved unchanged into `grrcruise_data.h`; all consumers use the actual state binding.

The existing stage-hazard component captures both arrays, including timers,
phases, velocities, rest heights and joint pointers. Its existing union has room
for this stage, so the complete component remains 5,728 bytes and the live census
still has 32 components. Dormant padding is retained as bytes; the original
initializer selectively initializes the fields it uses, and those decisions
have not been changed.

The owning GObj/Ground pair has generation and userdata-role checks. All 37 model
joints have retained class spans and admitted type identities. Collision IDs are
frozen by the scene lease; platform state tags and required joints validate.
The wind field is claimed from the actual Ground record through the dynamics
owner's physical-slot reference. There is no duplicate wind payload or live wind
mirror. Class, GObj, Ground and dynamics companions retain their own payloads.

Load validates the complete local candidate and then checks restored companion
agreement before copying either array. The Ground must still be map 1, own the
saved GObj, and bind both actual arrays and the claimed wind. No snapshot operation
allocates, runs platform callbacks, changes physics or draws. Candidate-wide
collision/graph agreement, other stage/attribute/world/HSD/HUD/effect storage,
Fountain reflection userdata and complete aggregate restore/resimulation/timing
remain separate work. Production retains the broad page oracle.

Evidence under `A:/FreakMelee-test-archives/`:

- `rainbow-oct11-owner-d`: 570 added checks and twelve malformed candidates;
  actual source moving-record initializer, platform updater and collision
  callback, actual owner snapshot/storage, all 37 joints, repeated sixteen-tick
  updates after restoration, callback/RNG counts, stale root/ground resurrection,
  late joint/type rejection, pointer/collision/tag/wind claims, overlaps, activity
  and retirement. Joint dirty/visibility, collision backend and RNG endpoints are
  controlled. The disappearing-record phase updates in this fixture are direct
  controlled writes, not the source disappearing-platform updater.
- `rainbow-oct11-gm-a`: connected production loop/control/player/camera fixture
  passes with the expanded component linked. Four build targets and focused
  GObj-consumer CTest pass.
- `rainbow-oct11-stages-a`: Rainbow Cruise, Castle and Fountain of Dreams
  old/current rendered CPU matches pass, 4,800 samples each, with exact populated
  traces and forty passing stage-hazard captures/validations per scene. Rainbow
  Cruise's sampled component mean Save is 6.147 us and Validate is 0.897 us.
  These are component means, not complete-match latency or worst-case bounds.
- `rainbow-oct11-eight-a`: stock and independent interpreted eight-actor
  scenarios pass all 32 components at ten boundaries each. Previous/current and
  diagnostic-on/off traces match; the live diagnostic performs no Load.
- `rainbow-oct11-independent-verification.json`: frozen/live input hashes,
  source bodies with explicit constructor substitutions, identical parameter
  layout and compiled actual `.mstate$m`/host `.data` sections verified.

Live rendered comparisons exercise stage gameplay and component Save/Validate;
they are not aggregate Load or full rollback performance acceptance.
