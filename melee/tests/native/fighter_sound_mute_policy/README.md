# Owned fighter sound mute policy source proof

The generic policy is now implemented as a candidate, with no game acceptance
claim. Live headers/runtime policy were compiled using the immutable64 compiler;
complete old sound bodies come from immutable64. Production integration/build
and the next immutable snapshot are owned by root.

The final standalone suite passed:

- 36,960 raw-stock/custom-definition old/current source pairs.
- 22,272 separate authored-mask and explicit-stop cases; 1,106,907 checks across
  the actual sound entries/commands/stops and actual metadata registry/resolver.
- 314 compiler programs with emitted files (310 without file emission): both
  routes, all 128 masks, decimal/hex/leading-zero decimal/comments, inherited
  masks, independent declaration preservation and strict invalid contexts.
- Four real generated-module console executables: both routes with an explicit
  127 mask and both with an omitted declaration, each tested against all 128 base
  masks. Base metadata is never mutated.

Receipt: `A:/FreakMelee-test-archives/sound-policy65-final/result.json`.
Reproduce into a new output directory:

```powershell
python tests/native/fighter_sound_mute_policy/run.py --root . --baseline .local/baseline64 --output A:/FreakMelee-test-archives/sound-policy65-root-repro
```

`fixture.py` mechanically extracts whole old/current seven direct channel entries,
all seven immediate stop bodies, the ordinary/positioned/pseudo-random command
handlers and the actual Definition/mask resolver. CommandInfo/native decoded
CmdUnion and NEXT_CMD are real shared definitions. Audio backend, ID mapping,
deterministic RNG and registry slot publication are explicit controlled boundaries.
The runtime-private capacity is extracted; the controlled slot records publish
only the fields accessed by the actual registry body. Their physical private
layout/model assets are not claimed.

Comparison reads individual retained handles and ordered initialized audio/RNG
journals. It does not compare fighter padding, digest masks, hardware mixing,
sound archives, or gameplay. All seven channels are compared through four routes,
including the unchanged explicit command stops10..15. Raw stock/default pairs
are separate from authored-mask contracts. Blocked authored channel0 retains
sound mapping and pitch RNG at331/332/370/371; pseudo-random selection happens
even when playback is muted. Unsupported random outcomes remain excluded as
specified in the source census below.

`source_manifest.json` is the historical pre-implementation census pin. The new
receipt hashes all actual code/header/compiler inputs and verifies no input
changed while the console suite ran. It intentionally has no mandatory CMake
dependency on the historical archive; root's normal grouped compiler/module
tests integrate separately.

## Historical source proposal (before implementation)

Read-only source proposal; no production change or acceptance run. The adjacent
manifest pins the live sources used for this census. This concerns the fighter's
`x2225_b6` filter, independently of the native client's `--mute` output gain.

## Exact current closure

`ftAction_80072320` (`ftaction.c`) decodes all four stage-sound words and maps the
sound ID before selecting behavior/channel 0..6. Only cases 2 and 6 inspect the
mute bit; when set, their raw GameWatch/Samus switch permits playback. Its audio
base is `player_id + x221F_b4` (do not accidentally change to the ordinary API's
`player_id * 2 + x221F_b4`). Direction/pitch and all parameters are already
decoded before the filter and command consumption must remain unconditional.

`ftAction_80071FC8` consumes all seven pseudo-random sound words and calls
`HSD_Randi(random_range)` before its channel switch. Channels 2/6 reach
`ft_800881D8`/`ft_80088328`; those contain the same raw-kind exception. This is
the other dispatch route, not a second place to consume RNG or a reason to add
an audio callback.

| Channel | Ordinary direct entry | Retained handle | Ordinary group base |
|---|---|---|---|
| 0 | `ft_PlaySFX` (`ft_0877.c`) | x2160 | global, no per-player group |
| 1 | `ft_80088478` | x214C | 0x36 |
| 2 | `ft_800881D8` | x2144 | 0x1E |
| 3 | `ft_80088510` | x2150 | 0x42 |
| 4 | `ft_800885A8` | x2154 | 0x4E |
| 5 | `ft_80088640` | x2158 | 0x5A |
| 6 | `ft_80088328` | x2148 | 0x2A |

Channel 2 ordinary playback stops channels 2 then 6 before assigning x2144;
channel 6 does the same before assigning x2148. No-op ID 0x83D60 does nothing;
stop ID 0x83D61 stops only the requested channel. Currently muted ordinary
fighters skip even those sentinel handling paths on channels 2/6. GameWatch and
Samus still execute them. Other channel helpers retain their own sentinel
handling; channel 0 has no equivalent sentinel special case.

Explicit stop functions `ft_80088770` through `ft_80088998`, bulk cleanup
`ft_80088A50`/`ft_80088C5C`, and existing handle clearing remain unconditional.
No mask is applied inside those stop primitives.
`ftCo_800886D8` uses the additional 0x72 group outside behaviors0..6 and is not
part of this seven-channel declaration; it remains unchanged.

All `x2225_b6` producers and consumers were checked: reset copies x2225_b5
(`fighter.c`), whose original value comes from `Player_GetMoreFlagsBit1`;
`ftCommon_80080460` sets the temporary filter and 80474 restores it;
`fn_800C8_inline` also enables it. `ftCo_800D38B8` separately filters its
`ft_8008805C` secondary request using the bit with no stock exception. That
secondary operation is outside the seven channel presentation policy and stays
unchanged. These are all current fighter-source references to the bit.

## Recommended data contract

Add immutable `uint8_t NativeFighterDefinition.sound_mute_channels`: bit N means
channel N respects x2225_b6. Valid bits are 0..6; reject bit 7/extra bits rather
than truncate them. No callback, new Host operation, mutable fighter field,
allocation, `behavior_kind`, `source_kind`, or stock handler delegation is needed.

Stock defaults are **0x44** for normal fighters (bits 2 and 6) and **0x00** for
GameWatch/Samus. Set these explicitly in copied-base definitions, including the
normal value in custom native bases such as Rook; do not treat zero initialization
as a normal default. No Rook content edit is proposed in this namespace.

Suggested global FMS declaration:

```text
sound_mute_channels 0x44
```

Accept one literal Int 0..0x7F before behavior declarations; reject duplicate,
negative, float, unknown/trailing tokens and out-of-range masks. Absence preserves
the copied base's immutable field. Both generation routes assign an explicit
declaration after copying the base, without resetting it when removing behavior
inheritance. This is not an imported DAT tuning field. Fingerprint the Definition
field/offset as normal; no legacy module-format accommodation is required.

Raw stock fighters have no active runtime Definition. A substantive pure runtime
lookup can use existing `Definition(kind)` first and `FindStockDefinition(kind)`
second, returning that field; stock families without copied bases and unavailable
custom definitions use the normal 0x44 mask. This uses the existing immutable
stock-base catalog, avoids reintroducing a raw GameWatch/Samus branch at the four
engine sites, and allows active generated definitions to override their bases.
Suggested real lookup name: `nativeFighterRuntime_SoundMuteChannels(FighterKind)`.
No wrapper around an audio function is proposed.

Apply a pure filter at the seven direct ordinary entries and at the positioned
subaction switch, after its original decoding/mapping. Only query metadata while
the mute bit is set. Remove the four old exception switches directly. Filtering
all seven entries is needed if authors are promised control of all seven bits;
changing only the current 2/6 conditions would make the other bits ineffective.
Preserve the existing audio argument order, sentinel handling, stop ordering and
retained-handle writes inside allowed paths. Blocked requests leave handles alone.

One additional concrete RNG constraint: `ft_PlaySFX` maps its sound then consumes
`HSD_Randi(200)` for IDs 332..370 after the play request and applies pitch. For an
author who newly filters channel 0, preserve that mapping and RNG decision even
when omitting the audio play/handle write/pitch calls. Otherwise a presentation
mask would unexpectedly alter gameplay's shared random stream. The stock defaults
do not filter channel 0, so this is an explicit authored-policy constraint rather
than a changed stock outcome. Pseudo-random command selection must likewise still
occur while playback is blocked.

## Bounded actual-source fixture plan

After contract approval, mechanically extract immutable old/current complete
`ftAction_80071FC8`, `ftAction_80072320`, all seven ordinary channel entries and
their immediate actual stop functions. Use real `CommandInfo/CmdUnion` native
decoded fields and NEXT_CMD, not hand-packed PPC bitfield assumptions. Compare
semantic command cursor, retained handles, ordered audio journals, sound mapping
and RNG calls. Audio backend, map result and deterministic RNG are controlled
boundaries; this does not prove hardware mixing, positional audio or sound-bank
selection correctness.

Healthy stock-equivalent matrix: every raw versus kind (Kirby is not being ported
but its existing normal sound behavior is still a valid reference), raw
GameWatch/Samus, corresponding custom generated definitions, mute on/off,
channels 0..6, ordinary/no-op/stop IDs, player IDs and partner bit, all four stage
pitch selections, all six valid pseudo-random outcomes and actual channel-0 pitch
RNG range boundaries. Compare exact stop order and stage-vs-ordinary group bases.
Keep unsupported pseudo-random range/output cases outside parity: the original
source leaves sfx_id uninitialized for an unsupported outcome.

Separate authored-policy matrix: all 128 masks, all channels and mute states;
isolated bits and all/none masks; mask override independent of active raw/custom
identity; no-op/stop and handle preservation; channel-0/pseudo-random RNG retention.
Test actual lookup default/catalog/active Definition selection. Strict compiler
negative and generated-module metadata tests cover both generation routes,
independent declaration and preservation when omitted. No destructive callback
or source retirement fixture is needed because the policy dispatches no script.

The initial proposal ended before any production edits or acceptance runs.
The implemented source/console evidence is reported at the top; root owns the
combined production build, defaults, public docs and final acceptance checkpoint.
