# Owned CPU attack profiles

[Fighter parameters](language.md#fighter-attributes) |
[Common action policies](common_policies.md) |
[Marth/Roy translation](../marth_roy_script_translation.md)

`cpu_profile.fcp` is a project-owned text file describing attack candidates
for Melee's shared CPU planner. A stock clone copies its seven tables and
distance threshold once from `PlCo.dat` when the project is created. The
file is included in the fighter package and restored when an installed
fighter is cloned. A match parses that package's file into fixed per-match
storage. It does not fetch the donor's CPU tables.

Editing this file changes the next package build without requiring a script
recompile. There is no CPU-table panel in the Workshop yet. The planner and
its common input-command scripts remain shared engine behavior. Fighter-owned
[CPU policies](cpu_policies.md) can replace motion classification, charge and
capture decisions with ordinary script code and queued input. This profile
file alone does not reproduce those fighter-specific decisions.

## File layout

```text
# family command prediction_frames x_min x_max y_min y_max weight period min_level
distance 22
ground 1 5 -2 15 -5 7 1 1 0
air 38 16 -5 10 -2 8 0.5 15 5
```

The example illustrates the format, not a complete stock profile. Lines can
contain `#` comments. There must be exactly one `distance` declaration in a
nonempty file. Each table can contain up to 32 candidates, in order. An empty
table needs no declaration. A missing or empty file means no authored attack
candidates and a zero distance threshold; there is no stock fallback.

| Field | Meaning |
| --- | --- |
| Family | `ground`, `air`, `ranged`, `smash`, `special`, `weapon`, or `edge`; selects one of the shared planner's seven candidate lists. |
| Command | Shared CPU input-command script index, `1..61`. This is not a fighter action or motion ID. |
| Prediction field | Integer timing field. Ordinary ground/air candidate tests use it as their lookahead frame count; other shared tests can derive timing from it. |
| X/Y bounds | Four float fields used by the corresponding candidate test. Ordinary tests scale the bounds by the fighter's size and mirror horizontal ranges with facing; ranged/weapon tests have their own calculations. |
| Weight | Float used in weighted candidate selection. Imported values are preserved, including negative values in two original Fox/Falco rows. |
| Period | Positive integer controlling the candidate's periodic eligibility test. Zero is rejected because the shared planner uses a remainder operation. |
| Minimum level | Lowest CPU level for the candidate, `0..9`. |
| Distance | Nonnegative finite threshold used by shared CPU approach logic. |

Floats accept ordinary decimal or C hexadecimal notation (`0x1.6p+4`). The
stock importer writes hexadecimal values so saving and loading preserves
every original binary32 value exactly. Non-finite floats, invalid command
indices, invalid levels, duplicate distance declarations, extra fields and
overfull lists are rejected. Clearing a list removes its candidates; row
terminators are generated in the fixed storage and are not written in the file.

## Importing defaults into an existing draft

```powershell
native_content_tool.exe import-cpu MY_PROJECT GAME.iso Marth
```

This explicitly replaces `MY_PROJECT/cpu_profile.fcp` with an independent
copy of the selected stock profile. The import is atomic and runs only when
requested. New stock projects already have this file. Importing defaults is
not required to keep a character with no authored CPU candidates valid.

## Verification and limits

The common-assets fixture imports all 32 original profiles, compares their
active rows and distance thresholds with the existing typed importer, and
checks an exact text round trip. Installed-clone acceptance preserves this
file after the author's draft is deleted and permits independent edits.
Selected stock-versus-script CPU matches are recorded in the translation
ledgers. Matching those recordings does not establish every planner branch,
CPU level, opposing fighter, or custom move's candidate tuning.
