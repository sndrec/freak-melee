# Owned pose metadata/compiler fixture

```powershell
python tests/native/source_pose_metadata/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/source-pose-metadata-review
```

Use a fresh output directory. Subprocesses are hidden; all compiled units use
-Wall -Wextra -Werror. Input hashes are checked before/after the bounded run.
No game/global build, template or user-project write is performed.

The actual direct source/program/expression compiler, complete binding catalog,
constants, metadata table/resolver, bytecode lowering and shared path validator
are linked. Source declares the exact owned Sheik hashes/slots/paths and all eight
pose operations; instructions retain opaque row selectors rather than addresses.
Native pose execution is explicitly unavailable in this compiler fixture. The
separate direct-pose runtime fixture must establish engine/VM behavior; this run
is not a DAT import, package activation or gameplay claim.

Checks cover source/facts order independence, full bounded syntax, exact tuples,
missing/unbounded/malformed facts, root-only/before-code placement, duplicate/hash
collisions, root value/helper/native/constant collisions, prepared-bank metadata
revalidation, foreign context/owner rejection, closed-selector type checking,
16-row boundary, selector clearing and failed-candidate preservation of the
accepted bank and every accepted buffer byte. Compiler request facts are a
controlled load-time input; no Definition, native offset or tick-time hash is used.

Final result: 237 checks across 57 compile attempts. Reserved literal/operator/
receiver names are rejected; source creator/opponent pose traversal, Article
creator access and Article/Effect borrowed graph names reject at real typed
resolution. The entity-negative cases use a controlled header validator only
to reach those owner checks, and prepare no entity runtime.
