# Scripted Popo/Nana companion pipeline

Uses the rebuilt live content tool and the frozen CPU-inclusive independent
`ice_climbers_activation/composition72/assemble.py` handoff.
The runner rejects an incomplete CPU fragment, missing independent declaration,
missing custom phase, reserved NULL phase replacement, or an old literal tuning
placeholder where the accepted source identifies a retail DAT value. It never
patches assembled source or overrides imported tuning.

Reproduction after root confirms matching tool/source handoffs:

```powershell
python tests/native/ice_climbers_scripted_pipeline/run.py --tool build/native/native_content_tool.exe --clang .local/baseline71/compiler/clang.exe --iso "C:/gayms/freak-melee/Super Smash Bros. Melee (USA) (v1.02).iso" --output A:/FreakMelee-test-archives/ice-climbers-scripted-pipeline72-root
```

`--prepare-only` writes the two final assembled source files and source census
without launching a compiler or content tool. It still requires the final source
handoff. Every output directory must be fresh and on A:. Text I/O is UTF-8 and all
subprocesses use `CREATE_NO_WINDOW`; compilation explicitly selects vcvars14.44.
`script-build` embeds live repository includes, so this is a captured **live**
source/header/tool proof, not a frozen71 ABI claim. Only the bundled Clang path is
reused; all production/header inputs are hashed before and after execution.

The actual pipeline is clone-stock for Popo and Nana, normal parameter import,
script-build using each project's actual code key, bundle-project, verify-package,
compose-family **companion**, and verify-package on the resulting family. It
creates one isolated content store. It never accepts a package into any store or
installs a package. `receipt.json` records both member package IDs, the family ID,
and `content_root`; the optional later runtime preparation must use the recorded
`accept FAMILY_ID ISO` command with that isolated root. Cached bundles are not
selectable until accepted. The installed Rook and user content store are untouched.

## Ownership and asset gates

`ownership.c` loads each actual generated DLL with the real copied production
base Definition. Raw table callbacks are failing fixture sentinels. Required Host
entries are nonexecuting sentinels; no gameplay callbacks execute in this probe.
Every original non-NULL custom phase must resolve inside that DLL (96 callbacks
per member), along with every non-NULL Definition callback, typed recipient export,
article callback, projectile callback, and article signal. Shared camera callbacks
remain the actual common table pointers. Original reserved rows363 and366 retain
four NULL slots; Nana retains NULL side/up special entry hooks and zero matching
special-state metadata, while both members own primary/secondary phase policies.

The source census requires actual CPU hooks, not fake empty substitutes: Popo
capture/pummel and Nana prepare/initialize/spacing/stick/drop/mode/output. The
probe checks each corresponding generated pointer. Original/generated behavioral
comparisons remain in the accepted component and follower CPU namespaces.

The FMP audit checks every import/literal declaration and schema count. It also
compares the accepted Ice/Blizzard retail-tuning facts against the real produced
DAT, so normal import cannot accidentally preserve fixture staging placeholders.
Fighter and article state names/types/counts/strides/array shape/bounds are checked
against final source; descriptors and export tables must reside in the member DLL.
The actual native CPU profile parser/importer compares each project's FCP against
its retail PlCo table. Sound bank13, effect bank14, mute0x44, ordered remaps/KO cue,
ignore-friction policy, demo root metadata and special mappings are checked.

Retail PlCo supplies the expected40 semantic bone pairs/49 runtime parts with no
omissions. The actual package verifier imports/registers/releases both members'
owned assets. The accepted raw-output audit is reused read-only, checking all
260 filled Nana actions, original8 actions,53 holes,199 referenced source bank
blobs, complete copied three-article graph and byte-preserved commands/flags/blend,
plus unchanged Popo raw assets. No materializer/copy helper is called by this audit;
only normal production clone construction produces the bytes.

## Limits

Root subsequently accepted the family only in its isolated store and completed
the hidden/muted rendered comparison. Both runs completed1,200 identical input
samples; the existing parity checker matched2,400 P1/P2 states,31 actions,
247 item states and1,200 RNG samples. Ice Shot, Squall, Blizzard and airborne
Belay all occurred. Receipts are under
`A:/FreakMelee-test-archives/ice-pair-runtime72-{stock,custom}/receipt.json`.
The stock/custom input SHA256 is
`f8ab70d22355c8cd9e164420c9e9d140296f08b3e4d70d084a80540a92b06246`.

`runtime.py` reproduces this batch; pass a fresh A: output, isolated content root,
and the receipt's accepted family ID as `--fighter`. Omit `--fighter` for stock.
Use `--compare-stock STOCK_GAME_LOG` on the custom run to require the existing
exact comparison and all four special transitions. The runner renders every
tick, uses `--hidden --mute --fast`, sends no OS keys, and never installs content.
`--prepare-only` writes the input file and command without opening the game.

The recorded player trace covers the primary entity, not a separate full Nana
state census. This batch contains no damage and does not prove capture, KO, Sleep
retirement, match-end/results, edited workshop roundtrip or rollback acceptance.
The earlier package receipt remains a cached-only pipeline record; later runtime
acceptance is deliberately separate.

Passing the package pipeline does not prove actual companion spawn/follow,
linked-special behavior, lifecycle execution, deterministic gameplay/rollback or
rendering. Those require a later isolated hidden/muted rendered batch authorized
and owned by root. Initial costume only is packaged. Verification tag counts also
include undecoded demo records; even a demo name matching an action symbol retains its original
separate-demo offset and is not counted as a decoded physical action. Legitimate
copied Popo/Taro retarget tags remain,
using immutable common rig metadata. This does not claim zero foreign tags or
fully decoded demo payloads. Existing source comparisons, import/retarget receipts
and source-only CPU limits remain distinct evidence.

## Preserved diagnoses

The first run at `A:/FreakMelee-test-archives/ice-climbers-scripted-pipeline72-worker-final`
passed Popo but found normal import incorrectly skipped all three Nana articles:
the importer selected the first same-name catalog row before checking source
kind. Root corrected the generic compound-key lookup using the existing explicit
Nana mappings. Neither assembled source nor imported tuning was patched here.

`worker-final2` then built/verified both members and passed their DLL probes, but
this fixture initially treated a same-name Nana demo Wait as an action-bank clip.
Its retail demo offset is intentionally unchanged and addresses a separate demo
archive. The corrected census validates physical offsets only for actions and
lists all populated demo records as undecoded metadata, with same-name aliases
explicitly marked. Both failed trees, logs, hashes and failure receipts remain.
These are authoring/audit diagnoses, not gameplay acceptance or asset mutations.
