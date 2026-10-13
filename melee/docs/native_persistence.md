# Native persistence boundary

The native client does not emulate a GameCube memory card. Melee's canonical
runtime data remains useful, but the card transport, card queues, slot probing,
card-error game mode, and snapshot-photo files are not part of the Windows
client.

## Runtime state retained

`gmMainLib_804D3EE0` remains the live owner of game settings, versus rules,
unlock flags, records, and other state consulted directly by original game
logic. `gmMainLib_8015FBA4` still initializes that data and applies the native
all-unlocked policy. Removing card I/O must not replace this structure or alter
its gameplay-facing reads.

The seven fighter-data records returned by `gmMainLib_8015CC4C` also remain
ordinary runtime state. Their presence in the original card manifest does not
make the card manifest their owner.

## Removed from native execution

The native startup and frame paths no longer:

- initialize Aurora's GameCube card backend;
- initialize the HSD card queue or bind the original save manifest;
- probe card slots during startup or every controller sample;
- initialize or poll the memory-card snapshot-photo service;
- install the boot-time card-error mode override;
- construct the card activity indicator for every scene; or
- reset and drain card queues at scene transitions.

These removals are made at the native call sites. They are not no-op card API
implementations. The original card modules still compile because unsupported
original modes reference them; they are dormant on the supported native
startup, versus, and direct-match routes.

Acceptance evidence: the full native client builds, all 58 `native_*` tests
pass, and the 10,000-controller-sample Pokémon Stadium replay reaches its end
and exits successfully after the removal. An intermediate run without the
snapshot/input-poll removal failed immediately in `CARDProbe`; that failure
confirmed the additional polling path rather than being treated as success.

## Native storage ownership

Native persistence should be split by ownership instead of serializing the
original `CardEntry` manifest:

- installation settings store language, audio/video options, default versus
  rules, and other machine-wide choices;
- player profiles store action bindings, tap-jump and short-hop options,
  C-stick behavior, calibration, and player-facing identity;
- the collection index stores installed characters and costumes for the whole
  installation, independently of profiles;
- replay files carry their own build, package, rules, seed, and input metadata.

The first concrete player-profile store now lives in `native_profile.c`.
Profiles use a bounded native representation for player identity, tap jump,
C-stick smash/tilt behavior, four physical bindings per action, optional
short-hop bindings, six analog-axis sources, and stick/trigger calibration.
An absent physical override means the connected controller keeps Aurora's
canonical default mapping, so a new profile preserves original controls.

Profile files live under `%LOCALAPPDATA%\Freak Melee\profiles` and are addressed
only by sanitized installation-local IDs. The fixed little-endian file rejects
bad magic, invalid enums/counts, truncation, trailing data, and checksum
changes. Saving writes a complete `.tmp` file, flushes it, then atomically
replaces the destination with write-through enabled. Loading decodes into a
temporary value and changes the caller's profile only after full validation.
The native profile test covers multiple action bindings, behavior options,
axis assignment, calibration, replacement, corruption, and path traversal.

The first installation-wide versus store is
`%LOCALAPPDATA%\Freak Melee\versus-settings.fms`. It saves the original live
`GameRules` values for match mode, time, stocks, handicap, damage ratio, stage
choice policy, friendly fire, pause, score and SD penalty, plus item frequency,
item selection, and random-stage selection from `GamePrefs`. Other fields in
those structures retain their normal runtime initialization. The 32-byte file
has explicit little-endian fields, a checksum, bounded values, exact-size
loading, and atomic replacement. Missing or invalid files leave Melee's
defaults untouched. Normal home boot loads it after the original defaults are
initialized; diagnostic direct-match launches bypass it.

The game saves local versus choices when CSS exits toward practice or stage
select. It does not save on process shutdown: an online guest's live rules can
temporarily be replaced by the room host's match setup. No persistence I/O runs
inside a match or rollback frame. Room setup snapshots the local versus fields
in memory before adopting the host's rules and restores them on the next CSS
entry. The focused settings test checks field
round-trips, preservation of unrelated runtime fields, items-off encoding,
room-rule restoration, replacement, corruption/truncation/trailing-data
rejection, and the isolated
installation path. A real home boot loaded a seeded 8-minute fixture from an
isolated `%LOCALAPPDATA%` root; the log reported mode 0, time 8, stocks 3.
The full-window Match rules page now edits the core game mode, timer, stocks,
damage ratio, item frequency, friendly fire, pause, and score display fields.
It saves them immediately, then applies the same values to the live original
rules. A live client switched Time to Stock and Medium to High items, saved,
restarted, and reloaded mode 1/item frequency 3 from an isolated store. Other
original settings such as the item mask and stage selection policy are
preserved by this editor for later controls.

Assigned profiles now resolve physical controls immediately after Aurora's SDL
sample and before the replay/rollback filter. Attack, special, jump, shield,
grab, and analog axes therefore enter gameplay, recordings, and network input
as the same canonical `PADStatus`. Each action may combine four physical
buttons or axis directions. A default binding preserves Aurora's canonical
sample, while an explicit binding replaces that action only. Calibration is
copied to the assigned controller at the profile transition rather than being
looked up or allocated during simulation. A dedicated short-hop binding is
carried in `PADStatus.extButton`, which the existing replay and netplay formats
already preserve. The focused input test covers default equivalence, combined
sources, action replacement, stick inversion/dead zones, trigger thresholds,
short-hop carriage, and clearing a profile.

Write persistent files only at explicit state transitions. Build the complete
new representation in memory, write a temporary file, flush it, and atomically
replace the previous file. Runtime simulation and rollback frames must never
perform file I/O. The current pre-release client does not need migration logic
for abandoned native formats.

## Remaining conversion work

The native Options screen now selects and edits these profiles. Startup also
accepts `--p1-profile` through `--p4-profile` for a profile ID in the
installation's native profile store, so
persisted bindings can already drive the ordinary menu path or a direct match.
Loading and calibration application happen once after controller
initialization; a missing or invalid file stops startup rather than silently
using different controls. Clearing or replacing an assignment restores the
controller calibration that preceded it. Aurora now exposes its standardized
physical button and axis state to Melee, and the native sampling layer uses that
boundary without serializing profiles through Aurora's global controller files.
Tap-jump disable and dedicated short hop now enter the common grounded and
aerial jump decisions. Their per-frame policy travels with the pad sample, so a
remote peer does not need the local player's profile file to simulate the same
result. Dedicated short hop also latches the short-hop decision when jump squat
begins, rather than depending on how long its physical source remains held.
C-stick tilt mode now leaves aerial attacks, SDI, throws, and other C-stick
consumers unchanged while redirecting the common grounded C-stick smash edges
to the existing forward-, up-, and down-tilt decisions. Direction and angled
forward-tilt selection come from the C-stick sample itself. The native control
tool can attach all three policy flags to an injected sample for deterministic
runtime diagnosis.

Item switch and random-stage selections retain their live mutations and enter
the versus store at the next CSS transition. Language, sound, deflicker,
vibration, and name entry also mutate live state without submitting the old
card request, but still need their own native persistence paths before they
survive a restart.

Original data deletion, trophy lottery, snapshot album/camera mode, Home-Run
Contest, Multi-Man Melee, Adventure/Classic/All-Star card updates, and debug
sound-test card operations are outside the current versus milestone. Their
card-backed flows should be removed with those modes or redesigned if any mode
is later brought into scope.

The GameCube card sources and Aurora card backend may be removed from the
native target once every native caller has been converted or removed. Do not
keep a compatibility adapter that serializes the old card image.
