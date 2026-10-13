# Interpreted fighter presentation, movement and attack scheduling

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Twenty additional root hooks now connect the authored program to the actual
native delivery sites. The directly indexed contract table has 73 entries; names
resolve during preparation, and callbacks use fixed actor/storage pools.

The presentation family covers four color-reset sites, five presentation-item
entries, taunts and demo entry. Demo construction attaches the source actor after
its model/parts are ready, runs Load, accepts only the native reset's deliberate
new life, then delivers presentation and the typed demo mode. A missing source
taunt uses the common taunt entry. Source actors never invoke donor callbacks.

Movement covers held-motion selection, neutral entry, air-resume selection,
grounded setup, held-timer expiry, manual/automatic aerial input and Wait cleanup.
Missing mapping hooks retain the incoming motion. Authored mappings must belong
to the published bank's common/owned layout. False aerial policies continue
common input; faults/retirement stop it. Parasol entry now guards continuation
after grounded setup and animation startup as well.

Attack scheduling covers both first-jab entry paths, rapid jab, down tilt,
aerial mapping/startup, forward-smash entry/startup and both crouch entry variants.
Common pickup priority stays intact. A replacement forward-smash entry takes
precedence over its post-common startup hook; it receives the original angle
after facing has been assigned. Missing hooks use shared entry behavior.

The generic lifetime contract was renamed directly from CPU to Fighter ownership;
there is no old-name alias or forwarding layer. It retains object pool/scene
identity, data, native generation, kind, program and script storage. Physical
retirement and a logical VM fault are both terminal for native continuation.

## Evidence and limits

- Presentation: `script-presentation-oct08-final/receipt.json`, 321 checks through
  the real compiler, preparation, actor, VM, context and Fighter/GObj pools.
  Model/costume/camera/audio and the common death-reset leaf are controlled.
- Attack/crouch: `script-fighter-attack-scheduler-review5/receipt.json`, 1015
  checks/102 cases, eight hooks, zero stock-handler dispatches. Actual dispatch
  and continuation bodies run against controlled ordinary engine leaves.
- Movement: `script-fighter-movement-state-intrinsics-final/receipt.json`, 892
  checks/eight hooks/64 lifetime cases, including real `state.wait` and
  `state.wait_blend` VM-to-native calls. The old state and transition fixtures
  still pass 277 and 451 checks respectively, with their documented boundaries.
- CPU after direct lifetime rename: 1786 ABI checks and 110 native scheduler
  sampler checks still pass, with their earlier controlled boundaries unchanged.

All receipts live under `A:/FreakMelee-test-archives/`. None establishes complete
fighter gameplay or renderer parity. Client/content-tool builds and all 36
selected interpreter/pool/lifecycle CTests pass after integration. The connected movement fixture also covers
the actual `state.wait` intrinsics. Its userdata-replacement case exposed a
binding continuation gap: the native entry stopped, but its outer binding could
publish success. State operations now retain their exact Fighter data pointer;
neutral entry additionally retains the full actor/program/storage lifetime.

The old primitive fixtures had synthetic source metadata without actual Fighter
actors. Their positive neutral checks were moved to the connected fixture, rather
than supplying a fake successful Run or weakening runtime guards. Fall/freefall,
landing, collision, numeric transitions and invalid-source checks remain there.

See the [owned stock-template admission census](native_source_roster_admission_census.md)
for package-level results. Generic engine primitives, legacy declaration removal,
source-owned resources/secondary models, the remaining scheduler families and
whole paired/roster matches still require work. Game & Watch's new normal-entry
schedulers alone do not implement its authored attack helper imports.
