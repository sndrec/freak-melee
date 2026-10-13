# Generic presentation controls

Registered compiler and generated-C fixtures cover both action-event and
explicit-state script forms. They test ordered single evaluation of RGBA
components, signed/high-bit packed colors and creation IDs, controller color
queries, exact material arguments, owned visibility masks, host capability
rejection, article layer enable/disable, opacity bounds, previous vertical
input and single-capsule disable bounds. Wrong types, duplicate/out-of-range
visibility slots and forbidden effect/event mutations are rejected.

The runtime fixtures execute actual generated code. Host operations are
controlled sinks; these fixtures do not claim to execute material rendering,
real visibility publication, full gameplay or a fighter translation. Separate
`fighter_visibility_assets` and `article_model_layers` fixtures exercise the
actual typed data/backend paths. The real registry/controller getter is checked
by `native_fighter_target`.

SDK61's release build and grouped run pass87 checks:
`.local/sdk61-build-ready.log` and `.local/sdk61-tests.log`.
All games remain unopened in this batch. Frozen61 has3,226 files, every hash
verified. The archive lives on A: through the `.local/baseline61` junction;
its contents remain immutable. The installed Rook was rebuilt for this SDK
without changes to behavior source or borrowed animation clips.
