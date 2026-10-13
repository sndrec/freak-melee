# Owned demo clip construction

This console matrix runs the actual generic demo copier, DAT accessors, Nana
article/action materializers, and the current shared animation-bank decoder.
It extracts retail inputs directly from the supplied v1.02 ISO. No game runs.

```
python tests/native/fighter_demo_copy/run.py --baseline .local/baseline72 --iso "../Super Smash Bros. Melee (USA) (v1.02).iso" --output A:/FreakMelee-test-archives/fighter-demo-copy-root
```

The support SDK/compiler is immutable baseline72. The copier/materializers and
shared decoder are actual current source, pinned separately in the receipt;
the decoder's updated table-list header is an explicit overlay. The runner
also pins the live local types header used by that decoder. Outputs must use
a fresh isolated A: directory. All subprocesses use CREATE_NO_WINDOW.

The corpus covers all 26 registered stock clone sources, excluding Kirby.
343 clips are populated: Mario/Luigi each have 15 of 16, Fox has all 14,
and the remaining 23 each have 13 of 14. Fox's slot 4 is
`PlyFox5K_Share_ACTION_Win2Wait2_figatree`; only the other sources leave it empty.

Results descriptors 0..9 come from the member's actual `GmRstM*.dat` root.
Intro 10..11 comes from `IrAls.dat`, ending 12 from `GmRegEnd.dat`, video wait
13 from `Pl*DViWaitAJ.dat`. Mario/Luigi 14/15 additionally use the exact
`Vi0102.dat`/`Vi1101.dat` roots. Copying these assets does not enable video
scene behavior. Nana's own results/intro/ending remain distinct; its video
wait is explicitly copied from Popo's source root, retaining Nana descriptor
flags and commands. Marth's stock vi filename uses lowercase final `j`;
DVD lookup is case-insensitive, and this extractor uses the exact FST spelling.

Checks cover every fighter byte outside populated descriptor offset/size words,
all original animation-bank bytes, each empty descriptor, exact nested retail
DAT bytes, actual decoded presence, and missing/overlapping/truncated/oversized
source rejection with unpublished output. Nana first runs the actual owned
article transplant and action-hole materialization, feeding their output into
the demo stage.

Historical first attempts are retained outside the namespace: exact-case Python
extraction first exposed Marth's spelling, missing overlay headers interrupted
compilation, and a test incorrectly assumed Fox slot 4 was empty. No production
copy policy was changed to mask those fixture errors.

The console receipt does not exercise project/CLI construction, editor saves,
results-scene creation, playback, rendering, or allocation failure injection.
Normal clone-stock construction is a separate integration gate owned by root.
