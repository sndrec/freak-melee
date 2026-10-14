# Glim

A little lantern spirit who fights by spending its own light. Glim starts
from Jigglypuff's rig and ordinary movement, with two aerial jumps, a teal and
brass costume, a floating petal halo, and new special moves and animation poses.

| Input | Move | Use |
| --- | --- | --- |
| Neutral B | Lantern Flash | Hold to charge, release for a short-range burst. |
| Side B | Firefly Skip | Aim a quick dash with the stick; one use per airtime. |
| Up B | Updraft | Rise on a burst of light, then enter special fall. |
| Down B | Wick Ward | Briefly reflect projectiles. A reflection adds charge to the next Flash. |

The project is an authoring example. Its damage, speed, timing and recovery
are provisional gameplay choices; it is not intended as a balanced tournament
fighter. Ordinary attacks retain the cloned base except for the documented
custom moves. The original stock fighter and existing user drafts are preserved.

## Build and edit

The checked-in files contain original source, tuning and generated artwork.
The local ISO supplies copied stock rig, clips and common assets. Generated DATs
and packaged copies stay outside Git.

Run `python characters/glim/build.py --iso "path/to/Melee.iso"` from the repo
root. The completed local project appears in the Workshop as **Glim**. The
builder refuses to overwrite an existing installed Workshop draft unless
explicitly told to replace it. Edit that draft freely; rerunning the recipe
does not replace your edits by default.

The [Workshop notes](WORKSHOP_NOTES.md) record authoring limitations found
during this character's creation, along with any workarounds used.
