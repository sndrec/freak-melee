"""Review source identity matching and import ownership on isolated fighter drafts.

No game windows or online services are opened. The companion native test checks
in-memory transaction failure and metadata serialization; this review checks
saved-project behavior across reordered, missing and ambiguous source exports.
"""
import argparse
import copy
import hashlib
import json
import os
import re
import shutil
import struct
import subprocess
from pathlib import Path

from review_native_skin_weights import make_glb


def fixture(path, *, reverse=False, slots=False, missing=False, duplicate=False,
            extra=False, bad_normals=False, rename_material=False):
    base = path.with_suffix('.base.glb')
    make_glb(base)
    data = base.read_bytes()
    length = struct.unpack_from('<I', data, 12)[0]
    document = json.loads(data[20:20+length])
    binary = data[28+length:]
    base.unlink()
    primitive = document['meshes'][0]['primitives'][0]
    document['materials'] = [dict(name='Red' if rename_material else 'Hull'), dict(name='Blue' if rename_material else 'Trim')]
    parts = [dict(copy.deepcopy(primitive), material=i) for i in range(2)]
    if bad_normals:
        del parts[1]['attributes']['NORMAL']
    if slots:
        document['meshes'] = [dict(name='Coat', primitives=list(reversed(parts)) if reverse else parts)]
    else:
        meshes = [dict(name='Hull', primitives=[parts[0]]),
                  dict(name='Hull' if duplicate else 'Trim', primitives=[parts[1]])]
        if missing:
            meshes.pop()
        if extra:
            meshes.append(dict(name='Accessory', primitives=[copy.deepcopy(parts[0])]))
        document['meshes'] = list(reversed(meshes)) if reverse else meshes
    document['nodes'] = [dict(mesh=i, translation=[i*3, 0, 0]) for i in range(len(document['meshes']))]
    document['scenes'] = [dict(nodes=list(range(len(document['nodes']))))]
    encoded = json.dumps(document).encode()
    encoded += b' '*(-len(encoded)%4)
    path.write_bytes(struct.pack('<III', 0x46546c67, 2, 28+len(encoded)+len(binary))+
                     struct.pack('<II', len(encoded), 0x4e4f534a)+encoded+
                     struct.pack('<II', len(binary), 0x004e4942)+binary)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('tool', 'test', 'draft', 'output'):
        parser.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    draft = output/'project'
    shutil.copytree(args.draft.resolve(), draft)
    assets = ('fighter.dat', 'animation.dat', 'costume.dat', 'assets.fma')
    original = {name: hashlib.sha256((args.draft/name).read_bytes()).hexdigest() for name in assets}
    receipt = dict(status='RUNNING', cases=[])
    paths = {}
    for name, options in [('original100%', {}), ('reordered', dict(reverse=True)),
                          ('missing', dict(missing=True)), ('duplicate', dict(duplicate=True)),
                          ('extra', dict(extra=True)), ('bad', dict(bad_normals=True)),
                          ('material', dict(rename_material=True)), ('slots', dict(slots=True)),
                          ('slots-reversed', dict(slots=True, reverse=True))]:
        paths[name] = output/(name+'.glb')
        fixture(paths[name], **options)
    env = dict(os.environ, LOCALAPPDATA=str(output/'appdata'))

    def run(exe, parts, success=True):
        result = subprocess.run([str(exe.resolve()), *map(str, parts)], env=env,
            cwd=output, capture_output=True, text=True, encoding='utf-8', errors='replace',
            creationflags=subprocess.CREATE_NO_WINDOW)
        with (output/'commands.log').open('a', encoding='utf-8') as log:
            log.write(' '.join(map(str, parts))+'\n'+result.stdout+result.stderr)
        assert (result.returncode == 0) == success, result.stdout+result.stderr
        return result.stdout

    def tool(*parts, success=True):
        return run(args.tool, parts, success)

    def sources():
        text = tool('project-model-sources', draft)
        return {key: int(mesh) for mesh, key in re.findall(r'^(\d+).* identity=(.*?) path=', text, re.M)}

    def plan(mesh, source, whole=True, success=True):
        result = tool('project-reimport-plan', draft, mesh, paths[source], 'whole' if whole else 'one', success=success)
        return {(int(p), int(m)) for p, m in re.findall(r'match primitive=(\d+) mesh=(\d+)', result)}

    def snapshot():
        return {name: (draft/name).read_bytes() for name in (*assets, 'workshop_names.txt') if (draft/name).exists()}

    try:
        native_draft = output/'native-project'
        shutil.copytree(args.draft.resolve(), native_draft)
        native = run(args.test, [native_draft, paths['original100%'], paths['bad']])
        receipt['native_checks'] = int(re.search(r'PASS (\d+)', native)[1])
        receipt['cases'].append('Native in-memory import rollback, frozen replacement identities and metadata codec')
        tool('project-reimport-plan', draft, 0, paths['original100%'], 'one', success=False)
        tool('project-import-meshes', draft, paths['original100%'], 0, 1, 'auto')
        found = sources()
        assert len(found) == 2 and 'mesh:4:Hull' in found and 'mesh:4:Trim' in found, found
        hull, trim = found['mesh:4:Hull'], found['mesh:4:Trim']
        expected = {(0, hull), (1, trim)}
        assert plan(hull, 'original100%') == expected
        assert plan(hull, 'reordered') == {(1, hull), (0, trim)}
        assert plan(hull, 'material') == expected
        assert plan(hull, 'extra') == expected
        assert 'original100%25.glb' in (draft/'workshop_names.txt').read_text()
        receipt['cases'].append('Persistent mesh identities survive source-file relocation, mesh reordering and material edits')
        # Display names belong to the editor; changing one leaves source identity intact.
        source_text = tool('project-model-sources', draft)
        offset = int(re.search(r'^'+str(hull)+r' mesh=0x([0-9A-Fa-f]+)', source_text, re.M)[1], 16)
        tool('project-label', draft, 'mesh', offset, 'Custom painted hull')
        assert plan(hull, 'reordered') == {(1, hull), (0, trim)}
        receipt['cases'].append('Renaming an editor mesh retains its source identity')
        before = snapshot()
        assert not plan(hull, 'duplicate', success=False)
        assert not plan(hull, 'missing', success=False)
        assert plan(hull, 'missing', whole=False) == {(0, hull)}
        assert snapshot() == before
        receipt['cases'].append('Ambiguous/missing whole-source matches fail without file changes; selected-mesh scope remains available')
        tool('project-import-meshes', draft, paths['bad'], 0, 1, 'auto', success=False)
        assert snapshot() == before
        receipt['cases'].append('A failed later primitive preserves all saved assets and source metadata')
        tool('project-import-meshes', draft, paths['slots'], 0, 1, 'auto')
        found = sources()
        coat_hull = found['mesh:4:Coat;slot:4:Hull']
        coat_trim = found['mesh:4:Coat;slot:4:Trim']
        assert plan(coat_hull, 'slots-reversed') == {(1, coat_hull), (0, coat_trim)}
        receipt['cases'].append('Named material slots survive primitive reordering and stay scoped to their source document')
        # Reject corrupt metadata at project admission, leaving saved files intact.
        names = (draft/'workshop_names.txt').read_bytes()
        try:
            (draft/'workshop_names.txt').write_bytes(names+b'mesh-source\t1\t4\t6\t0\t0\t0\t0\t1\t0\tpath\tkey\n')
            tool('project-model-sources', draft, success=False)
        finally:
            (draft/'workshop_names.txt').write_bytes(names)
        assert plan(coat_hull, 'slots-reversed') == {(1, coat_hull), (0, coat_trim)}
        receipt['cases'].append('Malformed source metadata is rejected on project open')
        receipt['status'] = 'PASS'
    except BaseException as error:
        receipt['status'] = 'FAIL'
        receipt['error'] = repr(error)
        raise
    finally:
        assert original == {name: hashlib.sha256((args.draft/name).read_bytes()).hexdigest() for name in assets}
        receipt['source_unchanged'] = True
        receipt['tool_sha256'] = hashlib.sha256(args.tool.read_bytes()).hexdigest()
        receipt['test_sha256'] = hashlib.sha256(args.test.read_bytes()).hexdigest()
        (output/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print('PASS: source identities, reimport matching, metadata admission and batch ownership')


if __name__ == '__main__':
    main()
