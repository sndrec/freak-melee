# Stage, tether-input and article-construction checkpoint

The shared native API now exposes direct stage-segment queries, global sound,
fast-fall operations, occupied-ledge lookup and second-phase common ledge jump.
The copied-ECB ledge probe is the only new substantive geometry operation:
it preserves the selected solver output/flag while leaving velocity changes
to the script. See [the scripting reference](scripting/stage_movement.md).

The optional `air_tether_input` callback replaces the complete stock predicate
at its existing interrupt priority. Its absence preserves stock behavior.
The dispatcher consumes an invalidated source rather than continuing through
the caller's cached fighter. The actual source fixture checks removal, address
reuse, kind change and definition replacement. It compares 2,560 absent-hook
input cases across all stock gates, five fighter kinds and both missing/present
definitions. Its copied-ECB probe checks the original left/right behavior,
including live ID outputs on failed probes, without claiming the controlled
stage solver itself is a gameplay comparison. Total: 5,183 checks.

Fighter and article scripts share typed segment/global-audio calls. Fighter
scripts additionally read/write position deltas, Z acceleration and tether
usage, read collision position, fast-fall state and special landing lag, and
invoke the movement operations above. The combined compiler fixture passes
158 checks. Both actual generated module forms pass 4,460 checks each across
128 field/query cases, required Host pointers, predicate returns and source
removal/reuse during second-phase ledge-jump entry.

Runtime `state.freefall` arguments now use typed expressions rather than the
former constant-only scalar parser. Its Int/Int/Bool/numeric/numeric contract
accepts the fighter's actual retained landing lag; the generated-module fixture
checks that value and source invalidation through a bound helper. The unused
old parser was removed. Tether-usage writes require Bool and position-delta
writes use the normal Float conversion rules.

`articles.create_with_velocity` can explicitly disable initial collision in
its spawn descriptor with a final Bool. Eleven arguments retain the ordinary
enabled default. Eighteen compiler checks cover both lowering forms, defaults,
true/false and malformed flags. The actual constructor fixture exercises both
descriptor values and retains its existing ownership/removal cases. Direct C
fixture callsites were migrated to the new signature.

Ness's immutable copied article bindings now include Bat slot 9 and YoYo slot
10, matching the original eleven OnLoad registrations. This copies assets;
it does not activate unregistered behavior. The default fighter templates and
previously accepted unregistered candidates are unchanged. Full Ness smash,
presentation, charge, ECB-sweep and resource lifecycle closure; Link tether
phase/capture composition; and Peach's article lifecycle remain separate
acceptance boundaries.

Validation: full native build, all 57 selected CTests and 2,199 freshly imported
Dr. Mario results-pill source comparisons pass. Frozen30 records 3,198 runtime,
SDK and compiler files. Frozen31 contains the final compiler corrections with
identical Host/runtime contracts and another passing full build/57-test batch.
Rook was rebuilt, verified and replacement-installed
with unchanged script, 285 own-rig clips and no borrowed clips. No game window
was opened. Its existing module also passes verification with the final31 tool;
this compiler-only correction requires no additional package replacement.
Local receipts live in `.local/stage-item-contracts-integration31`.
