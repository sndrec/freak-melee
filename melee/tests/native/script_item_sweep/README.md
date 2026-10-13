# Item segment sweep comparison

Run `python tests/native/script_item_sweep/run.py --compiler <clang.exe> --output A:/FreakMelee-test-archives/<fresh-directory>`.

The fixture executes the actual compiler, prepared Item program, Item actor/VM,
receiver storage and fixed GObj pools. The unchanged sweep, line existence,
normal, contact and moving-line speed/remap bodies are mechanically extracted
and checked against `d25ac9faa`. Native stage geometry accessors, joint lookup
and contact callback dispatch also execute, along with Aurora's actual native
normalization body. The `mpCheckMultiple` intersection leaf supplies known
hits/misses; the line geometry and contact callbacks are controlled. This
does not claim full stage geometry acceptance.
Owned Fighter actors and helper VM calls also exercise the two stage line queries.

Direct calls compare complete Item records, initialized normals and lines,
all 32 collision flag bits, and the hit output. Actual source callbacks use
both `item.state` and local destinations and check one evaluation of the
previous-position expression. Negatives cover output typing/distinctness,
physical identity, generation, scene, Article program ownership and storage.
Controlled non-destructive storage mutations probe output publication guards;
these are not arbitrary hostile native callbacks. A guard failure suppresses
script outputs while preserving any native position snap already performed.

Line comparisons cover normal sampling, floor/ceiling/wall contact dispatch,
exact collision record/category/amount/mode, velocity remapping, clamping and
collapsed previous segments. Invalid, disabled, hidden and missing lines are
bounded before unsafe native lookups. Controlled callback mutations verify
source retirement, structural geometry replacement and ordinary line hiding.
No full stock stage callback closure or hostile callback containment is claimed.

The runner pins inputs and emits a receipt. All compiler and fixture subprocesses
use hidden windows. No game, GPU or audio process is launched.
