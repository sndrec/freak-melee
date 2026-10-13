# Source-owned holdable visuals

The fixture runs the actual canonical PNG framing/CRC validator, Windows WIC
decoder, source visual preparation/release unit and holdable draw function. It
also extracts the actual immutable item-program pointer rebinder. No generated
authored C behavior is used.

Known 5×3 RGBA inputs verify every tiled channel byte, transparent/partial alpha,
32-byte alignment and edge padding. Draw observations check the translucent
pass, camera-facing coordinates, texture identity and exact source-spec owner,
bank, program identity, PNG and size agreement. The native numeric item fields
are deliberately varied behind the controlled trusted Spec boundary to show
that Draw does not use them as visual array indices; real actor identity
validation remains the production actor's responsibility.

The controlled program declarations use distinct identities derived with the
actual preparation formula (`package identity XOR golden-ratio constant ×
source owner`). Only their bank is shared. Records must retain each individual
identity; a shared identity requirement would incorrectly reject multi-entity
packages. Art-free zero-record owners also have an explicit release regression.

A recipe without a PNG uses its exact prepared Article model through the common
item draw operation. Missing/mismatched declared PNG visuals do not use that
path. Registry absence, wrong owned model and visual mismatches draw nothing.

Candidate failure after one successful image decode releases only candidate
pixels and leaves all candidate program visual pointers unbound. Accepted
pixels survive. Release checks selected-bank identity, exact Fighter/Item/Effect
references and idle VM/proc/draw/deferred work before freeing anything.

The source bank/program metadata, trusted actor Spec lookup, actor-reference
queries, camera and GX/model leaves are controlled. This tests resource and draw
contracts; it does not compile FMS, construct actors, initialize a renderer,
activate a complete package or establish gameplay/visual acceptance.

The PNG validator is structural. WIC success is separately required, but WIC
can tolerate malformed compressed data; successful decoding is not a strict
deflate-stream validator. The corruption regression uses a rejected CRC rather
than claiming a permissive decoder rejects all corrupt pixel streams.

```powershell
python tests/native/source_item_visual/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/source-item-visual-review
```

Use a fresh output directory. The runner launches every compiler/test subprocess
with `CREATE_NO_WINDOW`, fingerprints inputs and archives commands/results.
