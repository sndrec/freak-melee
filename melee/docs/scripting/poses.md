# Fighter poses

Pose operations copy, blend, and sample a fighter's skeleton. Use `Pose.live`
for the pose shown in the game. Use `Pose.blend` for a second pose that you can
prepare and then blend into the live pose.

## Pose graphs

A pose graph contains a set of bone transforms. Declare a name for each graph
before your functions, actions, moves, articles, or model effects:

```text
pose_graph GroundRest slot 4 path "0"
pose_graph AirRest slot 5 path "0"
```

The slot selects a graph in the fighter's assets. The path selects a bone inside
that graph: `""` selects the root, `"0"` selects its first child, and `"0/1"`
selects the second child of that first child. The selected bone and its following
child/sibling sequence form the pose. Graph names can contain up to 31 characters;
paths can contain up to 12 child indices, each from 0 to 255.

Each pose resource stores a list of [bone IDs](bone_queries.md#bone-ids), one for
each descriptor in graph order. Copying or blending applies each descriptor to
its listed bone. These IDs stay the same when you rearrange the skeleton. Newly
added bones keep their current pose until you include them in the list. Loading
checks that the graph and its bone list match and that every target bone exists.

## Applying a pose

```text
func BlendRest(float frame, float descriptor_weight, float pose_weight):
    pose.load_animation(Pose.blend, 305)
    pose.request_frame(Pose.blend, frame)
    pose.copy_costume(Pose.blend, 1)
    pose.evaluate(Pose.blend)
    pose.blend_graph(Pose.blend, GroundRest, descriptor_weight, pose_weight)
    pose.blend_to_live(1, descriptor_weight, pose_weight)
```

This example samples animation 305 into the alternate skeleton, blends in the
rest pose, and combines the result with the live pose. Animation IDs refer to
entries in this fighter's animation bank. The `start` arguments below are bone
IDs; `1` is the starting transform bone in this example.

| Operation | What it does |
|---|---|
| `pose.load_animation(destination, clip_id)` | Attach the selected animation's tracks to the skeleton. |
| `pose.request_frame(destination, frame)` | Set the animation frame to sample. |
| `pose.evaluate(destination)` | Apply the requested animation frame to the skeleton. Animation callbacks run during evaluation. |
| `pose.copy_costume(destination, start)` | Copy the costume's rest transforms, beginning at the selected bone. |
| `pose.copy_graph(destination, Graph)` | Copy the pose to its mapped bones in the selected skeleton. |
| `pose.blend_graph(destination, Graph, descriptor_weight, pose_weight)` | Combine the graph's transforms with the current transforms of its mapped bones. |
| `pose.blend_to_live(start, alternate_weight, live_weight)` | Combine the alternate and live skeletons, beginning at the selected bone. |
| `pose.copy_to_live(start)` | Copy the alternate skeleton into the live skeleton, beginning at the selected bone. |

For a usual blend, use weights that add up to 1. For example, `0.25, 0.75`
uses one quarter of the graph pose and three quarters of the current pose.
Each weight is used as written, so values outside 0 to 1 can extrapolate poses.
Copy operations use the game's rest-pose and model-scaling rules; use them when
you want to restore a pose completely.

Pose operations affect the fighter running the script. They change bone poses
while the current action continues. To change the action itself, use the
[fighter state operations](fighter.md). See [synchronous callbacks](synchronous_callbacks.md)
for how evaluation handles callbacks that remove or replace a fighter.
