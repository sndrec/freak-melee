# Glim: Workshop observations

This log separates actual UI use from batch asset authoring. Batch tools call
the same project backend, but they do not prove the corresponding UI is useful.

Initial observations:

- Cloning through the command-line tool copies assets but supplies no behavior
  file. The Workshop supplies a starter template separately. A standalone
  authoring recipe needs to provide its source explicitly.
- Models can import one glTF primitive at a time. A complete character with
  several materials requires repeated import review and Apply operations.
- The outliner uses `Mesh 0`, `Joint 0`, and similar fallback names for the
  stock costume. Choosing attachments requires consulting the rig role map.
- Removing unused stock special states and assigning new action names are
  separate from editing their scripts. The clone leaves a crowded action list.

The first source preparation accepts all 79 functions, checks every required
motion callback, and uses 850 interpreter instructions with no imported
attribute-offset declarations. The two aerial jumps and four specials use
owned parameters and state variables.

The offline animation helper rejected repeated stock clip names. Glim's recipe
now appends its own clips and updates only the authored action rows, preserving
every inherited offset and shared clip. New curves use sparse linear keys so
their handles remain practical to edit.

Timed-hitbox dogfooding exposed a runtime flag issue: retail reads the
victim-only flag from the next command. A duration word cannot safely serve
that role. Timed hitboxes now read their own header flag; stock conversion
copies the retail flag into that header. Ordinary retail commands keep their
original behavior.

More observations and completed workflow checks will be added as Glim is built.
