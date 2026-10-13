# Direct shared vector primitives

Canonical F32 APIs in Fighter and Article scope are `vectors.angle_xy(ax,ay,bx,by)`
(four inputs), `vectors.angle(ax,ay,az,bx,by,bz)` (six), and
`vectors.cross_normalized_x/y/z` (six). All return F32 and are pure. The former
Article-only six-input XY spelling is rejected; explicit source migration drops
its ignored z values. There is no compatibility overload or atan2 substitute.

The runtime calls actual `lbVector_Normalize`, `lbVector_NormalizeXY`,
`lbVector_AngleXY`, `lbVector_Angle`, and
`lbVector_CrossprodNormalized`. The last calls `C_VECCrossProduct` then normalizes
the result; it does not separately normalize both inputs. Three-dimensional
angle tests its length product against `1e-10f`; XY tests nonzero. These different
zero/tiny/nonfinite branches, cosine clamping, handedness, operation order and
F32 rounding are retained. No NaN/Inf/zero rejection or replacement is added.

The no-window runner mechanically extracts these unchanged native bodies and
the actual Dolphin cross-product body (excluding unrelated PPC assembly/render
functions) into its fresh A: output. Normal magnitude helpers and native
`sqrtf_accurate` definition come from current headers. Direct/top-level binding
results are compared bit-for-bit with native calls across zeros, signed zeros,
axes, arbitrary vectors, tiny/subnormal values, overflow magnitudes and NaN/Inf.
Actual source/statement/bytecode/VM exercises all five operations in both actor
contexts. Invalid contexts, six-argument XY and malformed formals reject.

Caller availability is controlled in this numerical console fixture so its
retirement checks can be exercised before output publication. Other native
families explicitly reject. This is actual native numerical/body and typed
dispatch evidence, not a live registry/scene/gameplay result.

Normalization uses three exact Float input/output formals in Fighter and Item
contexts. The fixture compares native output components (including signed zero,
tiny/overflow and nonfinite values), executes parser-to-VM local writeback, and
rejects literal/Int destinations and publication after caller retirement. The
generic call copies inputs before execution and only returns output formals
after all result/lifetime checks. No additional allocation is introduced.
