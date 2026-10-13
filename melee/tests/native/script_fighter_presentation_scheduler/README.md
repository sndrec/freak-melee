This console fixture extracts the actual color reset, presentation item, demo
entry and taunt delivery bodies and executes source hooks through the real
compiler, prepared Fighter program, actor and VM in native Fighter/GObj pools.
The native context/source guards are mechanically extracted from the real
properties unit; no successful actor, hook or VM entry is stubbed.

Asset/model/camera/audio and presentation initialization leaves are controlled.
The boundary never delegates an attached actor to stock fighter callbacks.
Evidence covers all four color reset sites (including both completion loops),
all five presentation entry methods, taunt/native player bookkeeping, absent
taunt handler common motion selection and actual demo constructor attachment.
Compiled source records Load -> Death/new-life -> PresentationItem -> DemoEnter
ordering, retains state across generation change and receives exact I32 demo mode.

Six identity mutations exercise native generation, layout/parameter identity,
scene epoch, kind and ft_data. Controlled primitive failures fault the real VM;
physical retirement really frees the GObj and Fighter pool slot. Color, each
presentation entry, taunt and demo creation stop before native continuation or
final bookkeeping on these failures. Unused graphics/stock constructors abort.

Demo asset initialization and native death/reset are controlled engine leaves.
The reset leaf deliberately changes native generation and uses actual actor
NewLife and Run(Death); it is not evidence for the original full death/reset
body or renderer/math parity. Other Item/Effect actors are explicitly absent.
No owned DAT, full clone, game or global build acceptance is claimed.

Final receipt:
`A:/FreakMelee-test-archives/script-presentation-oct08-final/receipt.json`.

The common-policy cohort adds ordinary `idle_variants`, `motion_is_angled` and
`worn_item_jump_sound` hooks at four actual shared scheduler sites, including
the multiple-jump path. The source compiler, hook contracts, prepared program,
actor storage and interpreter are real. Before-integration decision bodies are
extracted from `5a9b7e468`; 388 comparisons cover all queried motion IDs from
-2 through 375, idle table/frames/policy combinations and both ItemScrew sound
outcomes. Two multiple-jump sound cases check the ABI and post-entry path.
Thirty-two injected failures or lifetime mutations stop native continuation.
Only an actual primitive failure is required to fault the VM: a scheduler's
lifetime rejection does not imply that the VM itself must become faulted.

The weighted choice, native animation data/pose leaves and inner jump/physics
leaves are controlled. Full capture, root motion, particles, DAT appearance and
fighter gameplay are not covered. Current total: 4,233 checks. No game window
or audio is used. The runner now links current declaration, animation-origin,
sound-policy and asset-layout validators rather than their obsolete link set.
