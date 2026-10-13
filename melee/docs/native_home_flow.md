# Home selection and practice flow

Status: normal boot now reaches home CSS. START locks local choices and enters
practice; the sidebar Start match button opens stage select with those choices.
Local versus, online stage entry/cancellation, and a four-player Chef/Sheik/
Ice Climbers action run have reached results. Broader online acceptance remains
open.

## Grid stage foundation (2026-09-22)

`grpractice.c/.h` authors its own collision, spawn/camera points, and HSD mesh
descriptors. It uses a separate stage ID and a native stage-load callback,
without loading a retail stage archive. A generated, filtered I8 texture with
nine mip levels draws the fine grid on grey walls/floor; red center axes follow
the user's reference. Seven quads share one material. The allocation and texture
generation happen on stage entry, not during gameplay.

`--match --stage practice` exposes this foundation for local diagnostic play.
Online launch rejects this stage. Local stage IDs now use
16 bits; stock replay signatures remain unchanged. The terrain table includes
the new ground ID with standard terrain friction. Stage audio avoids indexing
beyond the retail audio table. The old `cam_vertical_tilt` name was corrected
to `vertical_fov_degrees`: its value is passed to the gameplay camera's FOV.

Explicit game and all-target builds passed. All 354 existing tests passed after
the final visual texture pass. Four-player runtime checks used human Ganondorf,
CPU Zelda/Sheik, Ice Climbers, and Mr. Game & Watch; movement, combat, camera
tracking, and the stage mesh were observed. The pause/exit chord returned to
the existing CSS with those selections retained. Screenshots reviewed:
`build/native/practice-stage-textured.png` and `practice-stage-exit.png`.
Those initial checks covered the stage foundation, before the practice behavior
below was added.

## Practice simulation (2026-09-22)

`native_practice.c` configures untimed free-for-all play, disables items, and
places the original Sandbag fighter in internal slot five. Its preload entry is
separate from the four local selections. Only those four selections get HUDs.
Practice changes the copied match setup, keeping the selected versus rules and
roster intact. Exiting skips the mode's results/bookkeeping path and returns
to the existing CSS. A single human is accepted by diagnostic practice launch.

Players use normal time-match KO/rebirth. Sandbag bypasses original blast-zone
death checks, so practice resets it explicitly beyond the bounds: fighter
cleanup releases grabs/effects, the center floor determines the respawn point,
damage is cleared, and ordinary falling/collision lands it. No per-frame
allocation or changes to normal character physics are involved.

Runtime evidence in `build/native`:

- `practice-sandbag-first.png`: Ganondorf, Zelda/Sheik, Ice Climbers plus Nana,
  Mr. Game & Watch, and a separate Sandbag running together.
- `practice-reset-frame-proof.log`: injected x=400 and 120% at the practice
  frame callback; next frame Sandbag was x=0, y=19.95, 0%, Fall. Subsequent
  observation showed it grounded and hittable again. An earlier reset attempt
  incorrectly treated the mutable player-position field as a spawn, asserted,
  and was replaced with explicit center-floor placement.
- `practice-player-rebirth.log`: after walking Ganondorf off the edge with
  virtual input, one fall, one stock still retained, Wait at x=-60 on the floor.
- `practice-nine-fighters.png` and `practice-nine-entities.log`: four local
  Ice Climbers selections plus Sandbag, nine distinct fighter entities alive.
  All four virtual ports received jump inputs. This does not prove four physical
  controller devices or the future profile/home-selector transitions.
- `practice-sandbag-exit.png`: normal pause/exit returned directly to CSS,
  with the four character choices retained and no dummy in the selectable roster.
- `practice-solo.png`: one human Ganondorf with the other local slots off,
  plus Sandbag; one human HUD and no timer.

Explicit game and all-target builds pass; all 354 existing tests pass. These
tests are regression coverage, not automated acceptance of the new home flow.

The user wants the game canvas to boot into simplified character selection.
START enters a grey grid practice stage with Sandbag and up to four local
players. The normal pause/exit chord (START, then L+R+A+START) returns to home
selection. B in play remains Special. Cancelling from home must never expose
the old menu hierarchy. Menus for configuration and larger management tasks
get a separate full-window layout; sidebars carry gameplay-useful information
and quick actions.

## Home integration (2026-09-22)

`native_home.c` owns application-side local selections and readiness, outside
rollback. The existing GM_VS state machine gains a Practice state rather than
copying character selection or introducing another frontend. Normal boot enters
its CSS; direct diagnostic matches remain available. Home B/back cannot enter
the legacy menu hierarchy. The CSS retains the original roster and cursor
presentation. The native home header replaces the retail rule caption with
"CHOOSE A FIGHTER" and hides the obsolete Back graphic; the Team Battle toggle
and four player doors remain. Broader CSS simplification is still pending.

START saves this PC's roster, preloads its fighters and Sandbag, and enters
practice. Practice modifies only the copied match rules. The normal pause/exit
chord returns to CSS, restores the local choices, and clears readiness. The
sidebar Start match button requires at least two local fighters offline. In a
room it requires host ownership and every active participant's confirmed roster,
with two to four fighters total. Spectators do not gate that readiness check.
There is no automatic start or second CSS. The host chooses the stage; existing
canonical setup/content agreement still precedes online simulation.

Room selections carry an explicit host start request. Cancelling stage select
returns both participants to their own home CSS without leaving the waiting
room. A disconnected start request cannot become an offline match accidentally.
The old `native_online_css` implementation and VS-menu room hooks were removed.
Room actions now run at home CSS/practice. Options is available in home CSS;
its input block and results port remapping run after Melee refreshes pad status.
Room readiness continues pumping while local practice is paused. Results uses
the unmapped physical GameStatus history to preserve button trigger edges.

Online home players can use any selected physical ports. The home selection
records those ports in ascending order; match input and results map them to the
room's assigned global human slots. Diagnostic direct-netplay ownership remains
explicit. Remote result controllers are marked absent so local acknowledgement
can finish.
Motor output now uses the same fixed logical-to-physical assignment. Remote
fighter slots have no motor route; the home-room route remains active through
results and returns to physical port numbering on CSS entry. Direct diagnostic
netplay resets its route at match shutdown. The focused output test covers a
global P2 fighter on physical port 3, muted remote slots, invalid duplicate
routes, and return to identity. Physical multi-controller rumble remains
unverified.

Different practice histories exposed two match initialization leaks: Camera_Init
retained bounds averages and pause/orbit state, and reused Fighter allocations
retained velocity/move fields. New camera/fighter instances are cleared before
normal setup. Field traces identified Zelda's xA4_unk_vel as the first remaining
fighter difference after fixing camera initialization. Attack/physics formulas
were not changed.

## Runtime evidence and limits

Evidence is under `build/native`; screenshots were visually inspected.

- `home-practice-first.png`: home START into solo practice. Pause exit restores
  CSS; holding B does not expose the original menus.
- `home-practice-two.png`, `home-local-sss.png`, `home-local-match.png`, and
  `home-local-results.png`: two local choices enter practice, sidebar Start
  opens SSS, then Battlefield, results, and home with choices preserved.
- `home-online-ready.png`, `home-online-host-sss.png`,
  `home-online-guest-sss.png`, and `home-online-cancel.png`: two clients practice
  independently, display Ready, and require the host button. Guest B cancels
  stage selection for both and clears readiness while preserving the room.
- `home-fighter-fix-host.log` / `home-fighter-fix-guest.log`: a three-fighter
  one-stock Battlefield match reached results, with 4026 / 3184 confirmed
  state checks and no rejected packets. This preceded the physical-to-global
  controller mapping fix; it does not prove correct local ownership then.
- `home-four-host.log` / `home-four-guest.log`, `home-four-inputs.png`: final
  mapping visibly controls the intended Zelda/Sheik, Ice Climbers, and G&W
  from guest physical ports 1..3. Idle simulation matched for roughly 2390
  checks, then diverged at frame 2397 following transformation/jump/Chef inputs.
  Both clients returned home. This historical failure was traced to the Chef
  item layout and is retained as the regression trigger.
- `home-options-return-verified.png`: after opening Options and injecting START
  and stick input, Back returns to the same home CSS rather than practice.
- `home-paused-ready.png`, `home-final-match-check.png`, and
  `home-four-final-results.png`: host started while practice was paused; all four
  fighters entered Battlefield. Guest physical ports 1..3 walked their assigned
  fighters offstage; both clients reached results (1773 / 1481 confirmed checks,
  zero rejected packets).
- `home-results-host.log` / `home-results-guest.log`: final build repeated the
  four-fighter movement/KO match with 1819 / 1804 matching confirmed checks.
  `home-results-return-host.png` and `home-results-return-guest.png` show both
  clients back at their own original CSS rosters after local result inputs.
  Remote slots acknowledge automatically; guest physical ports 1..3 each
  dismiss their own result panel. This used virtual input, not three physical
  devices. Longer injected START presses needed additional taps during this
  check; physical held-button behavior still deserves a targeted check.
- `chef-index-host-err.log` / `chef-index-guest-err.log`: both peers spawned the
  same Chef variant 4, then read it as invalid variants 527 and 439. A shared
  Game & Watch article helper wrote an eight-byte visibility pointer over the
  old four-byte pointer prefix and variant. The native item layout now gives
  the pointer its full width and places the Chef variant and Rescue owner after
  it. The semantic digest includes those active values/relations.
- `chef-verified-host.log` / `chef-verified-guest.log` and
  `chef-fixed-results.png`: a fresh delayed two-client run used Chef, Sheik,
  Ice Climbers, and Ganondorf inputs, corrected six/seven timelines, compared
  1423/2984 confirmed checksums, and reached the winner screen without a
  mismatch. This is local two-process evidence, not a cross-network proof.
- `trace6-host.log` / `trace6-guest.log` and
  `trace6-host-round2-return.png`: two clients kept the same room code and ICE
  connections after an aborted first match, then completed a two-minute second
  match. They compared 6360/6656 confirmed checksums without a mismatch, both
  dismissed results, and returned to practice with Start match enabled.
  Results input now checks the mapped logical port so the guest can dismiss its
  own panel. Home rooms retain signaling for the next match; direct CLI rooms
  still release it after setup.
- `trace5-host.log` / `trace5-guest.log`: a separate repeat-room run diverged
  at frame 2059 of its second match after 1919/1871 confirmed checksums. An
  earlier run diverged at frame 684. The failing state was not localized; the
  later successful run does not establish reliable repeat-round determinism.
- `disc-host.log`, `disc-host-after.png`, and `disc-rejoined.png`: forcing the
  guest process to exit during combat made the host time out after five seconds,
  return to CSS in the same room, and accept a replacement guest. Both readied
  up and launched another match, which diverged at frame 768. The corrected
  shutdown log recorded 990 received input packets before the disconnect.
- `trace8-*` through `trace14-*`: retained semantic-state traces localized
  repeat-round failures. Frame-zero differences came from a stage object's
  uninitialized `Ground.x0` and an unused `xC0` material value; the checksum
  now excludes the former and includes the latter only while its material flag
  is active. A later failure reached 645 synchronized frames before differing
  in item hitboxes. The next run reached 2576 frames and identified the third,
  disabled hitbox slot of a Food item as the first difference. The checksum now
  ignores payload bytes of disabled item hitboxes but still checks active ones.
  These are checksum corrections, not changes to Melee's collision routines.
- `trace15-*` and `trace16-*`: another repeat round failed at frame 1134 in a
  Bob-omb hurtbox after matching its identity, motion, animation, and hitboxes.
  Item hurtbox initialization does not write the fighter-only `bone_idx` or
  cached world positions; the original collision path refreshes those positions
  before use while `skip_update_pos` is clear. The digest now omits the unused
  index and hashes cached positions only when valid. A later two-client run
  completed a full first round with 7,435 guest-side confirmed checksums, then
  failed in round two at frame 1424 after the hurtboxes of a Capsule item
  matched. Its subsequent damage/timer/counter group was the first difference.
  That remaining mismatch needs field-level diagnosis before repeat-room
  determinism can be claimed.
- `trace17-*`: a later run briefly lost the first host's signaling connection;
  it rejoined the guest's surviving room under the same code, and both clients
  reached practice and match entry. Round one completed with 7436/7437
  confirmed checksums. Round two failed at frame 555 on a Crate's damage angle;
  the raw values differed while knockback was zero. Collision writes both
  fields together, so the digest now checks the angle only when knockback is
  active. The changed checksum has focused tests; the later `scopefix-*` run
  completed two consecutive rounds, while broader item coverage remains open.
- `nonconsecutive-repeat-host-ready.png` and
  `nonconsecutive-repeat-guest-ready.png`: the host selected Dr. Mario on
  physical port 1 and Peach on physical port 3, leaving port 2 empty; the
  guest selected one Dr. Mario. Both published Ready. In the logged Battlefield
  match, the host's port-3 input moved Peach in global fighter slot 2. The same
  port advanced Peach's results panel while host port 1 handled its own.
  `nonconsecutive-host.log` and `nonconsecutive-guest.log` record 7436/7437
  matching confirmed checksums, zero rejected packets, and no corrections
  across the full two-minute round. Both returned to the same room
  (`nonconsecutive-repeat-return.png`). This uses virtual input for port 3,
  not a second physical controller. An earlier unlogged Onett attempt returned
  to CSS shortly after match entry; its cause is unknown and is not counted as
  acceptance evidence.
- `room-count-mismatch.png`: a one-client room advertised two fighters but
  selected only one before practice. Start match stayed disabled and the rail
  displayed "Need 2 fighters. Exit practice to change." instead of the vague
  waiting message. The same roster validation controls readiness publication.
- `solo-teams-on.png` and `solo-teams-practice.png`: with Team Battle selected
  and only Fox locked in, START entered practice with Sandbag. Home CSS no
  longer applies the retail requirement for two opposing teams to this local
  free-for-all practice transition. The retail versus readiness rule remains.
- `teams-guard-same-practice.png`, `teams-guard-opposed-practice.png`, and
  `teams-guard-stage-select.png`: two red-team fighters entered practice, but
  Start match stayed disabled with a prompt to assign different teams. Changing
  P2 to blue enabled Start match; the sidebar button opened stage select with
  the locked-in fighters. Team validation covers the combined online room
  roster as well as offline selections; this run exercised the offline path.
- `repeat-before-fix-*`: a new two-client match initially rejected the host's
  controller assignment. The host owned global slots 0, 2, and 3, but slots 2
  and 3 were nonhuman; the new rumble map tried to reserve physical controllers
  for them. Nonhuman slots now remain unmapped and send neutral controller
  status. The subsequent two-client match entered gameplay.
- `cmdtrace-*`: a second-round checksum mismatch at frame 742 isolated a Super
  Scope item whose identity, motion, animation, hitboxes, hurtboxes, and damage
  matched. Only raw script scratch words `xDAC` and `xDB0` differed. Super Scope
  logic never reads `xDAC..xDB8`, and allocation leaves them unwritten, so the
  native semantic checksum excludes those four words for this item kind. It
  still checks active Scope flags and the same `xDAC` word for Dr. Mario's pill,
  whose logic uses it. A focused test covers both cases; original item logic
  was not changed.
- `scopefix-*`: two clients played two consecutive two-minute Battlefield
  rounds as Dr. Mario in one signaling room, passed through both results
  screens, and returned to CSS in the same room. Round one compared 7430 host
  and 7433 guest confirmed checksums without corrections. Round two compared
  6936 host and 7247 guest confirmed checksums without a mismatch; the host
  reported one short timeline correction, the guest two, and the guest rejected
  three input packets. One guest pill input and host jump/attack inputs were
  exercised. This is a repeat-room sample, not coverage of every item or
  character, and it does not prove that a Scope spawned during the passing run.
- `broad-*` and `acid-*`: a direct-UDP, four-human Brinstar run split two local
  slots per client and crashed in both processes when rising acid reached a
  fighter. The acid callback's collision descriptor is a relocated pointer at
  offset `0x2C` in `GrZe.dat`; the importer had copied it as a scalar, and the
  stage source read it through a GameCube-layout `HSD_GObj` cast. The importer
  now resolves the typed hit descriptor and the stage uses its named field.
  The archive test checks its 14 damage and 90-degree angle. A fresh one-minute
  four-fighter Brinstar run with high items reached time-out on both clients,
  compared 3186/3616 confirmed checksums without mismatch, rejected zero
  packets, and corrected one/three timelines. `acid-cycle-host.bmp` shows active
  play; `acid-results-host.bmp` shows the subsequent No Contest screen. All
  four players tied, and the native netplay gate at that point did not accept
  the separate sudden-death scene. This earlier run is a direct-UDP
  stage/rollback sample, not a completed winner path or a room-UI/cross-network
  test. A relocation audit of 37
  selectable-stage parameter blocks found no other undeclared generic
  parameter pointers; Icicle Mountain's three relocated fields use its
  separate typed importer. The generic importer now rejects an undeclared
  parameter relocation at load time, with a negative Brinstar archive test.
- `stock-brinstar-host.bmp` and `stock-brinstar-guest.bmp`: a second direct-UDP
  Brinstar run used the same four-human, two-client split and high items with
  one stock. Ganondorf won; both clients rendered the same winner and placement
  screen. The host/guest compared 1997/2129 confirmed checksums without a
  mismatch, rejected zero input packets, and corrected zero/one timelines.
  This confirms the stage fix through a winner result, but does not itself test
  a timed tie or prove a room-UI/cross-network match.
- `sudden3-*` and `sudden3-result-*.bmp`: a one-minute, four-human Battlefield
  tie entered Melee's separate sudden-death scene on both direct-UDP clients.
  The existing connection stayed open while a fresh input/checksum timeline
  began; no new RNG seed was applied. Both clients reached the same Ganondorf
  winner screen. The first scene compared 3784/3100 confirmed checksums; sudden
  death compared 1885/1892. Neither scene reported a mismatch, rejection, or
  rollback correction. Each client ignored one late packet from the prior
  scene. This verifies a local direct-UDP transition; room-UI and cross-network
  sudden death remain untested.
- `room-sudden-*` and `room-sudden-result-*.bmp`: the same timed tie ran through
  the local signaling service and ICE room transport. Signaling was released
  after setup. Both clients entered sudden death on the established connection
  and displayed the same Ganondorf winner screen. They compared 3182/3818
  confirmed checksums in the first scene and 1721/1469 in sudden death, with
  zero mismatches, rejected packets, or rollback corrections. One late
  old-scene packet per client was ignored. This exercises command-line room
  setup, not the sidebar create/join flow or play across two networks.
- `ui-sudden-*` and `ui-sudden-{host,guest}-end.png`: two normal home clients
  used the sidebar to create and join room `8KA9QJKT`, selected Dr. Mario,
  readied in practice, and used the host's Start match button to open stage
  select. A two-minute Battlefield tie entered sudden death, where the guest
  walked offstage. Both clients displayed Dr. Mario as the winner. The first
  scene compared 5976/5977 confirmed checksums; sudden death compared
  1643/1643. Neither client reported a mismatch or rollback correction, and
  neither rejected an input packet. The host ignored two late packets from the
  previous scene. `ui-sudden-host-after-start.png` and
  `ui-sudden-guest-return.png` show both clients back at CSS with the same room
  code and two participants; readiness resets for the next round. This is a
  same-PC sidebar room test with one fighter per client, not cross-network or
  physical shared-controller acceptance.
- `css-caption2-first.png`, `css-caption2-team-on.png`,
  `css-caption2-practice.png`, and `css-caption2-return.png`: the native home
  caption is centered in the existing Melee header, Team Battle remains
  usable, the caption disappears in practice, and it returns after the normal
  pause/exit chord. The gameplay canvas has no new overlay.
- Release game and all-target builds passed. All 354 tests passed serially;
  selection tests cover start-request encoding and invalid flag combinations;
  semantic-state tests cover the Game & Watch pointer/variant/owner layout.
  These tests do not replace online runtime acceptance.

## Remaining acceptance

- Broaden action and stage coverage beyond the completed Chef/Sheik/Ice
  Climbers run, including physical shared-PC controllers and cross-network play.
- Exercise repeat rounds with varied characters, items, inputs, and changed
  stage/rules; investigate any future checksum mismatch or packet rejection.
  Spectator host/start, rotations, sidebar sudden death, and physical
  multi-controller rumble verification remain open.
- Room readiness accepts N selected human slots in arbitrary physical ports,
  with all other local slots off. A mismatched local count is explained in the
  rail; physical multi-controller testing remains open.
- Continue simplifying the original CSS presentation, especially controls
  unrelated to local fighter selection. Review profiles, four physical
  controllers, and narrow/high-DPI layouts.
- Preserve direct-match/replay behavior; recheck changed selections, disconnect,
  repeated practice/real-match transitions, and all local result confirmations.
