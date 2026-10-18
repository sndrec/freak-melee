# Workshop bone pose checks

`run.py --output <fresh-directory>` extracts the production bone pose reader and
writer plus HSD's quaternion matrix and matrix-to-Euler functions. It links the
current Aurora matrix library. Actor selection, part count and flag/dirty
notifications are controlled test boundaries.

The cases compare the rendered rotation with independently calculated Euler
matrices, check read-only queries, vary stale quaternion W values across identical
manual edits, preserve unrelated flags and SRT, and reject invalid inputs. The
hidden native workflow review covers real actors, asset refresh and rendering.
