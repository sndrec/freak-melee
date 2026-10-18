# Owned fighter pose graphs

Pose operations use the fighter's own immutable graph recipes and animation
bank. They manipulate live or alternate joint poses directly without entering a
motion, changing animation bookkeeping, or selecting fighter-specific policy.

Declare graphs before helpers, actions, moves, articles or model effects:

```text
pose_graph GroundRest slot 4 path "0"
pose_graph AirRest slot 5 path "0"

func BlendRest(float frame, float descriptor_weight, float pose_weight):
    pose.load_animation(Pose.blend, 305)
    pose.request_frame(Pose.blend, frame)
    pose.copy_costume(Pose.blend, 1)
    pose.evaluate(Pose.blend)
    pose.blend_graph(Pose.blend, 1, GroundRest, descriptor_weight, pose_weight)
    pose.blend_to_live(1, descriptor_weight, pose_weight)
```

There are at most 16 named graphs. Slots are 0..15 and request the corresponding
owned imported joint graph. A path is an empty string for its root, or up to 12
slash-separated child ordinals (each 0..255 with at most three digits), within 63 characters. `"0"`
selects the first child. Names have the ordinary 31-character identifier bound;
duplicates and hash collisions are rejected. Each declaration must match its
package's exact owned name-hash, slot and path. The interpreter binds names to
the existing asset rows, independent of declaration or row order. Scripts receive no
descriptor pointers. Graph slots must satisfy the active asset layout and
package validation; a declaration does not bypass those checks or duplicate
the imported graph allocation.

`Pose.live` and `Pose.blend` are literal destinations. Operations do not accept
an arbitrary integer destination or a dynamically selected graph name. Direct
part starts are Int expressions containing the fighter's
[bone IDs](bone_queries.md#bone-ids). Omission parts are a separate mapping that the shared
traversals consume. A graph path is not an omission or a new runtime part.

| Operation | Behavior |
|---|---|
| `pose.load_animation(destination, clip_id)` | Install tracks from an own clip into live or alternate joints. Does not request/evaluate a frame or change motion bookkeeping. |
| `pose.request_frame(destination, frame)` | Request that frame recursively on the selected whole skeleton. Evaluation is separate. |
| `pose.evaluate(destination)` | Evaluate the selected whole skeleton, including animation end callbacks. |
| `pose.copy_costume(destination, start)` | Resolve the costume descriptor corresponding to the selected runtime part and copy it into the selected skeleton. |
| `pose.copy_graph(destination, start, Graph)` | Copy the declared descriptor subtree into the selected skeleton. |
| `pose.blend_graph(destination, start, Graph, descriptor_weight, pose_weight)` | Blend the descriptor into the selected skeleton with two independently supplied weights. |
| `pose.blend_to_live(start, alternate_weight, live_weight)` | Blend alternate joints into live joints. |
| `pose.copy_to_live(start)` | Copy alternate joints into live joints. |

Weights are neither normalized nor clamped, and the second weight is not
implicitly computed from the first. Full-weight blends remain blends because
the actual copy/blend paths have different flags, quaternion and scale handling.
Negative finite frames and weights are allowed. Float inputs are evaluated once
in written order and checked for finiteness. The owned clip resolver validates
the registered animation-bank ownership/span and clip ID before reading its
table; missing, replaced or invalid banks yield no operation. Part/descriptor/clip
topology, omission walks, destination joints, and owned asset spans are validated
before the real shared operation; failure leaves poses untouched. Argument
expression side effects already evaluated are not rolled back on validation
failure. Oversized generated statements reject compilation rather than truncate.

These operations apply only to the source fighter. They are unavailable through
recipient relations such as `creator.pose` or `opponent.pose`: a graph selector
belongs to the program that declared it. Article and model-effect callbacks cannot
dereference a fighter through this surface. Costume copying uses the actual
costume-to-part resolver rather than assuming every start means the costume's
first child. Graph copying uses the declared subtree directly.

Native execution keeps resolved clip/graph pointers only within one synchronous
operation. Load, copy, blend and evaluation calls have conservative source
lifetime guards; an invalidating call stops the current helper/callback before
subsequent fighter or pose access. Requesting a frame is a pure request update.
Nested argument calls follow the ordinary precise-site lifetime lowering, so a
removed fighter cannot be read by a later argument. See
[synchronous callbacks](synchronous_callbacks.md).

The direct interpreter checks are in `tests/native/source_pose_metadata`,
`script_pose_catalog` and `script_pose_runtime`. They cover owned tuple binding,
closed selector types, bytecode lowering, all eight native routes, independent
weights and retirement during native calls. Runtime asset views, topology
validation and pose math are controlled in the routing fixture; its result does
not establish Sheik Chain gameplay or animation fidelity. An ordinary
`source-check` also accepts an eight-operation helper using the real copied
Sheik asset graph. Complete Sheik migration and live gameplay remain separate
acceptance gates. The older generated-C fixtures are reference material only.
