"""Run with Blender --background --disable-autoexec FILE --python SCRIPT -- ...

Exports evaluated local transforms to editable figatrees, including constraints.
The named armature must match the costume's preorder and rest hierarchy. The
root basis conversion is inferred and verified; sheared transforms are rejected.
"""
import argparse
import json
import math
from pathlib import Path
import sys

import bpy
from mathutils import Matrix, Euler, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
from fighter_animation_io import Dat, joints, figatree, float_track


def trs(t, r, s):
    return Matrix.Translation(Vector(t)) @ Euler(r, 'XYZ').to_matrix().to_4x4() @ Matrix.Diagonal((*s, 1))


def error(a, b):
    return max(abs(a[i][j]-b[i][j]) for i in range(4) for j in range(4))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--costume', type=Path, required=True)
    parser.add_argument('--joint-symbol', required=True)
    parser.add_argument('--armature', default='Armature')
    parser.add_argument('--actions', nargs='+', required=True)
    parser.add_argument('--frame-counts', type=Path,
                        help='JSON map of action names to output lengths; preserves existing game timing')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args(sys.argv[sys.argv.index('--')+1:])
    frame_counts = json.loads(args.frame_counts.read_text()) if args.frame_counts else {}
    if set(frame_counts) - set(args.actions):
        raise ValueError('Frame counts contain an action that is not being exported')
    if any(type(value) is not int or value < 0 for value in frame_counts.values()):
        raise ValueError('Frame counts must be nonnegative integers')
    costume = Dat(args.costume.read_bytes())
    skeleton = joints(costume, costume.roots[args.joint_symbol])
    rig = bpy.data.objects[args.armature]
    bones = []
    def walk(bone):
        bones.append(bone)
        for child in bone.children:
            walk(child)
    for bone in rig.data.bones:
        if bone.parent is None:
            walk(bone)
    if len(bones) != len(skeleton):
        raise ValueError('Costume and Blender joint counts differ')
    correction = trs(skeleton[0]['translation'], skeleton[0]['rotation'], skeleton[0]['scale']) @ bones[0].matrix_local.inverted()
    for i, (bone, joint) in enumerate(zip(bones, skeleton)):
        parent = bones.index(bone.parent) if bone.parent else -1
        if parent != joint['parent']:
            raise ValueError(f'Hierarchy mismatch at {bone.name}')
        local = bone.parent.matrix_local.inverted() @ bone.matrix_local if bone.parent else correction @ bone.matrix_local
        if error(local, trs(joint['translation'], joint['rotation'], joint['scale'])) > 0.002:
            raise ValueError(f'Rest transform mismatch at {bone.name}')
    # Clear action leftovers before each clip; unkeyed properties use rest pose.
    rig.animation_data_create()
    for track in rig.animation_data.nla_tracks:
        track.mute = True
    args.output.mkdir(parents=True, exist_ok=True)
    report = dict(bones=[b.name for b in bones], actions={})
    for name in args.actions:
        rig.animation_data.action = None
        for pb in rig.pose.bones:
            pb.matrix_basis = Matrix.Identity(4)
        action = bpy.data.actions[name]
        rig.animation_data.action = action
        end = frame_counts.get(name, math.ceil(action.frame_range[1]))
        values = [[[] for _ in range(9)] for _ in bones]
        previous = [None] * len(bones)
        worst = 0.0
        for frame in range(end + 1):
            bpy.context.scene.frame_set(frame)
            evaluated = rig.evaluated_get(bpy.context.evaluated_depsgraph_get())
            for i, bone in enumerate(bones):
                pb = evaluated.pose.bones[bone.name]
                local = pb.parent.matrix.inverted() @ pb.matrix if pb.parent else correction @ pb.matrix
                t, q, s = local.decompose()
                r = q.to_euler('XYZ', previous[i]) if previous[i] else q.to_euler('XYZ')
                previous[i] = r.copy()
                residual = error(local, trs(t, r, s))
                worst = max(worst, residual)
                if residual > 0.003:
                    raise ValueError(f'{name} frame {frame} {bone.name}: unrepresentable shear {residual}')
                for channel, value in enumerate((*r, *t, *s)):
                    values[i][channel].append(float(value))
        nodes = [[float_track(channel, track) for channel, track in zip((1,2,3,5,6,7,8,9,10), bone)] for bone in values]
        # TopN is the engine-owned world transform, not an animation channel.
        if bones[0].name != 'TopN':
            raise ValueError('Expected engine-owned TopN as the first joint')
        nodes[0] = []
        path = args.output / (name + '.dat')
        path.write_bytes(figatree(name, nodes, end))
        channels = list(action.fcurves)
        throw_keys = [k.co.x for c in channels if c.data_path.startswith('pose.bones["ThrowN"]') for k in c.keyframe_points]
        report['actions'][name] = dict(frames=end, source_frames=math.ceil(action.frame_range[1]),
            release=max(throw_keys) if throw_keys else None,
            max_transform_error=worst, file=path.name)
    (args.output/'manifest.json').write_text(json.dumps(report, indent=2))
    print(json.dumps(report['actions'], indent=2))


if __name__ == '__main__':
    main()
