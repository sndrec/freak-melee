# Shared interpreted vector math

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The direct interpreter now calls the actual shared Melee routines for
`vectors.angle_xy(ax,ay,bx,by)`, `vectors.angle(ax,ay,az,bx,by,bz)` and the three
`vectors.cross_normalized_x/y/z` components. All inputs/results are F32, copied
through local vectors. These pure operations work in Fighter and Article bodies,
allocate nothing and expose no geometry pointers or fighter-specific decision.

Native XY tests a nonzero length product; native3D angle tests greater than
`1e-10f`. Cross uses native cross-product order and normalization. No alternate
atan2 formula, finite guard, epsilon rule or fallback direction was added.
Native signed-zero/subnormal/NaN/Inf/overflow results and Float rounding remain.

XY now consistently takes four arguments in both source contexts. Samus's old
Article call and the signal fixture were updated explicitly; their ignored Z
arguments had no side effects. The test-only Item C oracle parses the same four
arguments and supplies zero Z; its three-dimensional angle remains six args.
The old six-argument XY form is rejected rather than retained as compatibility.

`A:/FreakMelee-test-archives/script-vectors-oct08-root-final/receipt.json` records
7764 checks, including2560 bit-exact direct/top-catalog versus native pairs and
Source->Statement->Bytecode->VM routing for every operation in both contexts.
Actual mechanically extracted unchanged `lbVector` bodies and Dolphin cross
code execute; unrelated engine leaves and source liveness are controlled.
This is numerical/source evidence, not live scene/actor/gameplay acceptance.

Client/content tool builds and36 selected interpreter/content/pool CTests pass.
Three additional test-only oracle checks pass after rebuilding their DLLs:
Article operations/compiler, Fighter script and blaster/Article signals parity.
The Article parser test checks canonical XY output and rejects XY6. An initial
assertion placement in the test was corrected; a stale generated fixture DLL
was rebuilt before its runtime check. No production failure was hidden.

Actual private-owned-data Zelda preflight advances from vector angle287 to
`cpu.clear_target`380. Sheik still needs Chain metadata/resources1543. Complete
forms, mutable ownership/child lifecycle and full runtime acceptance remain open.
The relation audit is [recorded separately](native_source_article_lifecycle_audit.md)
and does not claim attach/detach implementation. No game, installed content or
public/user draft migration occurred. All commits remain local.
