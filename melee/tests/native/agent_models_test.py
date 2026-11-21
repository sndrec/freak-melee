"""Exercise packaged model authoring on real owned fighter assets."""
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools'))
from review_native_gltf_sources import fixture


def snapshot(root):
    return {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in root.rglob('*') if p.is_file()}


def model_checks(call, target, root, *, live=False, settled=None):
    """The same authored operations must work offline and in the live owner."""
    read = 'session.models.read' if live else 'models.read'
    edit = 'session.models.edit' if live else 'models.edit'
    info = call(read, dict(target, view='info'))
    assert info['bones'] > 10 and info['meshes'] > 0 and info['maximum_vertex_influences'] == 16
    bones = call(read, dict(target, view='bones', limit=200))['bones']
    assert len(bones) == info['bones'] and bones[0]['parent'] == -1
    assert all('bone_id' in row and len(row['position']) == 3 for row in bones)
    first_page = call(read, dict(target, view='bones', limit=3))
    assert len(first_page['bones']) == 3 and first_page['next_offset'] == 3
    assert call(read, dict(target, view='bones', query=bones[5]['name']))['bones']
    baseline = snapshot(root)
    def change(operation, **fields):
        if live and settled: settled()
        revision = call(read, dict(target, view='info'))['revision']
        arguments = dict(target, revision=revision, operation=operation, **fields)
        if live: arguments['summary'] = 'Model authoring: ' + operation
        result = call(edit, arguments)
        assert not result['installed'] and result['saved'] != live
        return result
    def reject(operation, **fields):
        if live and settled: settled()
        before = snapshot(root)
        revision = call(read, dict(target, view='info'))['revision']
        arguments = dict(target, revision=revision, operation=operation, **fields)
        if live: arguments['summary'] = 'Reject model authoring: ' + operation
        error = call(edit, arguments, expected=False)
        assert snapshot(root) == before
        assert call(read, dict(target, view='info'))['revision'] == revision
        return error
    def history_roundtrip(before, after, operation):
        if not live: return
        settled()
        state = call('session.inspect', target)
        undone = call('session.undo', dict(target, revision=state['revision']))
        assert call(read, dict(target, **operation))[next(iter(before))] == next(iter(before.values()))
        settled()
        undone = call('session.inspect', target)
        call('session.redo', dict(target, revision=undone['revision']))
        assert call(read, dict(target, **operation))[next(iter(after))] == next(iter(after.values()))
    # Labels and source metadata are restored with the assets, in one step.
    meshes = call(read, dict(target, view='meshes', limit=200))['meshes']
    label = change('label', category='mesh', id=0, name='Agent mesh')
    assert label['changed']
    labeled = call(read, dict(target, view='meshes', limit=200))['meshes']
    assert labeled[0]['name'] == 'Agent mesh'
    history_roundtrip({'meshes': meshes}, {'meshes': labeled}, dict(view='meshes', limit=200))
    assert not change('label', category='mesh', id=0, name='Agent mesh')['changed']
    stale = dict(target, revision=info['revision'], operation='label', category='mesh', id=0, name='Stale')
    if live: stale['summary'] = 'Refuse stale label'
    assert call(edit, stale, expected=False)['code'] == 'revision_conflict'
    # Rendering settings must also update native owner draw-pass flags.
    materials = call(read, dict(target, view='materials', limit=200))['materials']
    mat = next(row for row in materials if row['surface_editable'])
    material = mat['id']
    result = change('material', material=material, properties=dict(
        diffuse=[20, 80, 180, 255], alpha=.625, shininess=24,
        translucent=True, disable_depth_writes=True,
        pixel_engine=dict(enabled=True, write_color=True, write_alpha=True,
            depth_test=True, write_depth=False, blend_operation=1,
            source_factor=4, destination_factor=5)))
    assert result['applied_properties']['diffuse'] == [20, 80, 180, 255]
    after = call(read, dict(target, view='materials', limit=200))['materials']
    assert after[material]['properties']['pixel_engine']['enabled']
    assert not after[material]['properties']['pixel_engine']['write_depth']
    history_roundtrip({'materials': materials}, {'materials': after}, dict(view='materials', limit=200))
    reject('material', material=material, properties=dict(translucent=False, disable_depth_writes=True))
    assert call(read, dict(target, view='materials', limit=200))['materials'] == after
    assert not change('material', material=material, properties={'alpha': .625})['changed']
    rendered_bones = call(read, dict(target, view='bones', limit=200))['bones']
    owners = {row['bone'] for row in meshes if row['material'] == material}
    assert owners and all(rendered_bones[bone]['flags'] & 0x100000 for bone in owners)
    change('mesh_material', mesh=0, material=material)
    change('label', category='material', id=material, name='Agent surface')
    textures = call(read, dict(target, view='textures', material=material))['textures']
    assert textures
    textured = change('texture', material=material, layer=0, texture=dict(
        rotation=[0, .25, 0], scale=[1.25, .75, 1], offset=[.125, .25, 0],
        wrap_u=1, wrap_v=2, repeat_u=2, repeat_v=3, magnification=0, blend=.875,
        lod=dict(enabled=True, minification=1, anisotropy=1, bias=.25, clamp_bias=True, edge_lod=False),
        combiner=dict(enabled=True, color_operation=0, alpha_operation=0,
            color_inputs=[15, 15, 8, 128], alpha_inputs=[7, 7, 4, 67],
            constant=[128, 64, 32, 255], register0=[20, 30, 40, 255],
            overrides=[False] * 12, color_expression=True, alpha_expression=True)))
    assert textured['applied_texture']['scale'] == [1.25, .75, 1]
    assert textured['applied_texture']['combiner']['color_inputs'] == [15, 15, 8, 128]
    assert textured['applied_texture']['lod']['bias'] == .25
    texture_after = call(read, dict(target, view='textures', material=material))['textures']
    history_roundtrip({'textures': textures}, {'textures': texture_after}, dict(view='textures', material=material))
    assert not change('texture', material=material, texture={'blend': .875})['changed']
    animated_material = None
    for candidate in call(read, dict(target, view='materials', limit=200))['materials']:
        sequences_before = call(read, dict(target, view='texture_sequences', material=candidate['id']))['texture_sequences']
        if sequences_before and sequences_before[0]['layer'] is not None:
            animated_material = candidate['id']; animated_layer = sequences_before[0]['layer']; break
    assert animated_material is not None, 'Mario should have an expression texture sequence'
    animated_texture = call(read, dict(target, view='textures', material=animated_material))['textures'][animated_layer]
    assert not animated_texture['animation_id_editable']
    bad_id = (animated_texture['properties']['animation_id'] + 1) % 8
    assert reject('texture', material=animated_material, layer=animated_layer,
        texture={'animation_id': bad_id})['code'] == 'texture_edit_rejected'
    reject('texture', material=animated_material, layer=animated_layer, texture={'animation_id': 37})
    remapped = call(read, dict(target, view='texture_sequences', material=animated_material))['texture_sequences']
    assert remapped == sequences_before
    chosen = remapped[0]
    change('texture_image', material=animated_material, sequence=chosen['id'], image=0, path=str(root / 'portrait.png'))
    reject('texture', material=material, texture={'combiner': {'color_operation': 3}})
    reject('texture', material=material, texture={'combiner': {'color_inputs': [16, 0, 0, 0]}})
    reject('texture', material=material, texture={'combiner': {'color_inputs': [15, 15, 8, 10]}})
    reject('texture', material=material, texture={'repeat_u': 0})
    bones = call(read, dict(target, view='bones', limit=200))['bones']
    # A late invalid bone in a rest batch must discard earlier valid edits.
    rest = list(bones[5]['position']); rest[0] += .25
    reject('rest', changes=[dict(bone=5, position=rest), dict(bone=511, position=[0, 0, 0])])
    assert call(read, dict(target, view='bones', limit=200))['bones'] == bones
    change('rest', changes=[dict(bone=5, position=rest)])
    posed = call(read, dict(target, view='bones', limit=200))['bones']
    assert abs(posed[5]['position'][0] - rest[0]) < 1e-5
    history_roundtrip({'bones': bones}, {'bones': posed}, dict(view='bones', limit=200))
    # Extra stable IDs survive descriptor movement and return on Undo/Redo.
    added = change('rig', rig_operation='add', parent_id=bones[0]['bone_id'], name='AgentExtra')
    expanded = call(read, dict(target, view='bones', limit=200))['bones']
    extra = next(row for row in expanded if row['bone_id'] == added['result_bone_id'])
    assert extra['extra'] and len(expanded) == len(posed) + 1
    history_roundtrip({'bones': posed}, {'bones': expanded}, dict(view='bones', limit=200))
    change('rig', rig_operation='reparent', bone_id=extra['bone_id'], parent_id=bones[4]['bone_id'])
    change('rig', rig_operation='remove', bone_id=extra['bone_id'])
    assert len(call(read, dict(target, view='bones', limit=200))['bones']) == len(bones)
    reject('rig', rig_operation='remove', bone_id=bones[4]['bone_id'])
    # Reordered exports match source identities; paint and rest compensation
    # preserve bind-space shape and source-aware weights.
    source = root.parent / 'model-original.glb'
    updated = root.parent / 'model-reordered.glb'
    bad = root.parent / 'model-bad.glb'
    fixture(source); fixture(updated, reverse=True, deformed=True); fixture(bad, bad_normals=True)
    scanned = call('models.scan', {'path': str(source)})['primitives']
    assert len(scanned) == 2 and scanned[0]['has_normals']
    imports = [dict(primitive=0, bone=0, binding='automatic'), dict(primitive=1, bone=0, binding='automatic')]
    with tempfile.TemporaryDirectory(dir=root.parent) as loose_dir:
        folder = Path(loose_dir)
        model = folder / 'model' / 'fighter.gltf'
        parts = model.parent / 'parts'
        parts.mkdir(parents=True)
        glb = source.read_bytes()
        json_end = 20 + struct.unpack_from('<I', glb, 12)[0]
        document = json.loads(glb[20:json_end])
        buffer = glb[json_end + 8:]
        (parts / 'mesh data.bin').write_bytes(buffer)
        document['buffers'][0]['uri'] = './parts/mesh%20data.bin'
        model.write_text(json.dumps(document), encoding='utf-8')
        assert call('models.scan', {'path': str(model)})['primitives'] == scanned
        (folder / 'outside.bin').write_bytes(buffer)
        document['buffers'][0]['uri'] = '../outside.bin'
        model.write_text(json.dumps(document), encoding='utf-8')
        reject('import', path=str(model), imports=imports)
        document['buffers'][0]['uri'] = './parts/mesh%20data.bin'
        uv = struct.pack('<8f', 0, 0, 1, 0, 0, 1, 1, 1)
        (parts / 'mesh data.bin').write_bytes(buffer + uv)
        document['buffers'][0]['byteLength'] = len(buffer) + len(uv)
        document['bufferViews'].append(dict(buffer=0, byteOffset=len(buffer), byteLength=len(uv)))
        document['accessors'].append(dict(bufferView=len(document['bufferViews']) - 1,
                                          componentType=5126, count=4, type='VEC2'))
        document['meshes'][0]['primitives'][0]['attributes']['TEXCOORD_0'] = len(document['accessors']) - 1
        document['textures'] = [dict(source=0)]
        document['materials'][0]['pbrMetallicRoughness'] = dict(baseColorTexture=dict(index=0))
        document['images'] = [dict(uri='../portrait.png')]
        (folder / 'portrait.png').write_bytes((root / 'portrait.png').read_bytes())
        model.write_text(json.dumps(document), encoding='utf-8')
        error = reject('import', path=str(model), imports=imports)
        assert 'base-color texture' in error['message'], error
    previous_info = call(read, dict(target, view='info'))
    reject('import', path=str(bad), imports=imports)
    change('import', path=str(source), imports=imports)
    current_info = call(read, dict(target, view='info'))
    assert current_info['meshes'] == previous_info['meshes'] + 2
    sources = call(read, dict(target, view='sources', limit=200))['sources']
    assert len(sources) == 2 and len({row['source_identity'] for row in sources}) == 2
    mesh = sources[0]['id']
    vertices = call(read, dict(target, view='vertices', mesh=mesh, limit=200))
    assert len(vertices['vertices']) == 4 and vertices['corner_count'] == 6
    vertex = vertices['vertices'][0]
    skeleton = next(row['id'] for row in bones[5:] if row['flags'] & 1 and
        row['id'] not in {weight['bone'] for weight in vertex['influences']})
    paint = change('paint', mesh=mesh, bone=skeleton, centers=[vertex['position']], radius=100, strength=.5)
    assert paint['affected_vertices'] > 0
    painted = call(read, dict(target, view='vertices', mesh=mesh, limit=200))['vertices']
    assert all(abs(sum(w['weight'] for w in v['influences']) - 1) < 1e-6 for v in painted)
    assert any(w['bone'] == skeleton for v in painted for w in v['influences'])
    rest_bones = call(read, dict(target, view='bones', limit=200))['bones']
    compensation = list(rest_bones[skeleton]['position']); compensation[1] += 1
    change('rest', changes=[dict(bone=skeleton, position=compensation)])
    compensated = call(read, dict(target, view='vertices', mesh=mesh, limit=200))['vertices']
    assert all(abs(a - b) < 1e-4 for v, other in zip(painted, compensated) for a, b in zip(v['position'], other['position']))
    assert [v['influences'] for v in compensated] == [v['influences'] for v in painted]
    plan = call(read, dict(target, view='reimport_plan', mesh=mesh, path=str(updated), whole_source=True))['matches']
    scan_updated = call('models.scan', {'path': str(updated)})['primitives']
    expected_matches = {row['id']: next(p['id'] for p in scan_updated if p['identity'] == row['source_identity']) for row in sources}
    assert len(plan) == 2 and {row['mesh']: row['primitive'] for row in plan} == expected_matches
    change('reimport', path=str(updated), mesh=mesh, whole_source=True)
    reimported = call(read, dict(target, view='vertices', mesh=mesh, limit=200))['vertices']
    assert [v['influences'] for v in reimported] == [v['influences'] for v in painted]
    # The existing PNG portrait is a real local image, embedded into owned data.
    current_materials = call(read, dict(target, view='materials', limit=200))['materials']
    current_meshes = call(read, dict(target, view='meshes', limit=200))['meshes']
    no_uv_material = next(row['material'] for row in current_meshes if row['id'] == mesh)
    reject('texture_image', material=no_uv_material, path=str(root / 'portrait.png'))
    material = next(row['id'] for row in current_materials if row['name'] == 'Agent surface')
    change('texture_image', material=material, path=str(root / 'portrait.png'))
    sequences = call(read, dict(target, view='texture_sequences', material=material))
    assert isinstance(sequences['texture_sequences'], list)
    # Applying an image preserves a material's authored coordinate mapping.
    # Reflection can use untextured geometry, while switching it to UV must
    # reject the whole change when that geometry lacks the required attribute.
    def named_material(name):
        return next(row['id'] for row in call(read, dict(target, view='materials', query=name))['materials'] if row['name'] == name)
    reject('mesh_material', mesh=mesh, material=material)
    change('texture', material=material, texture={'mapping': 1})
    change('mesh_material', mesh=mesh, material=material)
    reject('texture', material=named_material('Agent surface'), texture={'mapping': 0, 'coordinate_source': 4})
    if live:
        assert snapshot(root) == baseline, 'Live model editing wrote saved files'
    final_info=call(read,dict(target,view='info'))
    return {'bones': len(bones), 'meshes': final_info['meshes'], 'materials': final_info['materials']}


def main():
    tool, iso = (Path(p).resolve() for p in sys.argv[1:3])
    with tempfile.TemporaryDirectory(prefix='Agent model authoring ') as temporary:
        root = Path(temporary); (root / 'userdata').mkdir()
        windows = Path(os.environ['SystemRoot'])
        env = dict(os.environ, LOCALAPPDATA=str(root / 'userdata'),
            PATH=str(windows / 'System32') + ';' + str(windows))
        def call(method, arguments=None, expected=True):
            result = subprocess.run([str(tool), 'agent', 'call', method, '--stdin'],
                input=json.dumps(arguments or {}), env=env, cwd=root, capture_output=True,
                encoding='utf-8', timeout=90, creationflags=subprocess.CREATE_NO_WINDOW)
            assert result.stdout, (method, result.returncode, result.stderr)
            reply = json.loads(result.stdout)
            assert reply['ok'] == expected, (method, arguments, reply)
            return reply['result'] if expected else reply['error']
        stock = call('stock.list', {'query': 'Mario'})['fighters']
        mario = next(row for row in stock if row['name'] == 'Mario')
        project = root / 'Mario'
        call('project.clone_stock', {'fighter_id': mario['fighter_id'], 'iso': str(iso),
            'output': str(project), 'title': 'Model Mario'})
        result = model_checks(call, {'project': str(project)}, project)
        # Paired members retain ownership of their individual costumes.
        zelda = next(row for row in call('stock.list', {'query': 'Zelda'})['fighters'] if row['name'] == 'Zelda')
        family = root / 'Zelda'
        call('project.clone_stock', {'fighter_id': zelda['fighter_id'], 'iso': str(iso),
            'output': str(family), 'title': 'Model family'})
        before = snapshot(family)
        info = call('models.read', {'project': str(family), 'member': 'partner', 'view': 'info'})
        call('models.edit', dict(project=str(family), member='partner', revision=info['revision'],
            operation='label', category='mesh', id=0, name='Partner mesh'))
        after = snapshot(family)
        assert {name for name in after if before.get(name) != after[name]} == {'partner/workshop_names.txt'}
        print('PASS: model import/reimport, actual weights, rest compensation, materials, textures, stable extra bones, atomic failure and member isolation', result)


if __name__ == '__main__': main()
