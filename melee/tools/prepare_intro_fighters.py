"""Build independent fighters and outfits for the startup movie.

Uses the ordinary Workshop authoring APIs through native_content_tool. All
generated DATs and installed packages stay in the chosen production directory.
    The optional costume DATs are supplied by the filmmaker; credits live in
docs/native_intro_production.md.
"""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('tool', 'iso', 'output', 'portraits'):
        parser.add_argument('--' + name, type=Path, required=True)
    for name in ('nook-island', 'nook-sweater', 'cowboy-green', 'cowboy-blue'):
        parser.add_argument('--' + name, type=Path)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    authoring = output / 'fighters'
    authoring.mkdir(exist_ok=True)
    content = output / 'content'
    receipt = {'content': str(content), 'fighters': {}, 'costumes': {}}
    transcript = []

    def tool(*parts):
        command = [str(args.tool.resolve()), '--root', str(content), *map(str, parts)]
        result = subprocess.run(command, capture_output=True, text=True,
                                encoding='utf-8', errors='replace',
                                env=dict(os.environ, LOCALAPPDATA=str(output / 'tool-profile')),
                                creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
        transcript.append({'command': command[3:], 'exit': result.returncode,
                           'stdout': result.stdout, 'stderr': result.stderr})
        (output / 'authoring-log.json').write_text(json.dumps(transcript, indent=2))
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        return result.stdout

    def clone(key, base, title):
        project = authoring / key
        if project.exists():
            raise ValueError(f'Choose a fresh output; draft already exists: {project}')
        tool('clone-stock', args.iso.resolve(), base, project, title)
        tool('project-stock-template', project)
        return project

    def package(key, project, portrait):
        shutil.copyfile(args.portraits.resolve() / f'stock-{portrait}.png', project / 'portrait.png')
        revision=json.loads(tool('agent','call','resources.read',json.dumps(dict(project=str(project)))))['result']['revision']
        tool('agent','call','resources.edit',json.dumps(dict(project=str(project),revision=revision,
            changes=[dict(operation='import',name='ui/player-card.png',kind='png',path=str(args.portraits.resolve()/f'card-{portrait}.png'))])))
        tool('script-build', project, project / 'behavior.fms')
        result = tool('package-project', project, project / 'portrait.png')
        identity = re.search(r'\b[0-9a-f]{64}\b', result)[0]
        tool('install-project', project, identity)
        receipt['fighters'][key] = {'id': identity, 'project': str(project)}
        (output / 'fighters.json').write_text(json.dumps(receipt, indent=2))
        print(f'{key}: {identity}', flush=True)
        return identity

    def costume_roots(data):
        _, size, relocations, public, external = struct.unpack_from('>5I', data)
        table = 32 + size + 4*relocations
        strings = table + 8*(public+external)
        result = []
        for i in range(public):
            _, name = struct.unpack_from('>II', data, table+8*i)
            start = strings+name; end = data.index(0, start)
            result.append((start, end, bytes(data[start:end])))
        return result

    def bind_costume(model, key, reference):
        data = bytearray(model.read_bytes())
        names = {name[name.index(b'_Share'):]: name for _, _, name in
                 costume_roots(reference.read_bytes()) if b'_Share' in name}
        for start, end, name in costume_roots(data):
            if b'_Share' not in name: continue
            target = names.get(name[name.index(b'_Share'):])
            if target is None or len(target)>end-start:
                raise ValueError('Costume symbols need a larger string table: '+str(model))
            # Bind color-specific public symbols to this draft's canonical
            # symbols. Leave the model, textures and all offsets unchanged.
            data[start:end] = target + b'\0'*(end-start-len(target))
        target = output/(key+'.dat');target.write_bytes(data)
        return target

    mario = clone('Mario', 'Mario', 'Mario')
    package('Mario', mario, 0)
    edited = authoring / 'MarioRemix'
    shutil.copytree(mario, edited)
    (edited / 'installed.id').unlink(missing_ok=True)
    for offset in ('0x719C', '0x71B4', '0x71CC'):
        tool('project-hitbox', edited, 55, offset, 'radius', 1536)
    # A complete keyed pose from Mario's own aerial jump becomes the fair's
    # windup, using the same pose-copy feature available in Animation mode.
    tool('project-pose-paste', edited, 18, 12, 69, 19)
    source = edited / 'behavior.fms'
    text = source.read_text()
    assert text.count('            item.gravity()') == 1
    text = text.replace('            item.gravity()',
                        '            velocity.y = velocity.y + 0.08')
    source.write_text(text)
    values = edited / 'fighter_parameters.fmp'
    text = values.read_text()
    assert 'float It_Kind_Mario_Fire.launch_speed = 1.5' in text
    # Keep stock launch speed so the upward arc stays in the demonstration's
    # camera rather than racing out of view before its acceleration is visible.
    package('MarioRemix', edited, 0)
    ganon = clone('Ganondorf', 'Ganondorf', 'Ganondorf')
    tool('project-hitbox-power', ganon, 1, 100, 400)
    package('Ganondorf', ganon, 25)
    falco = clone('Falco', 'Falco', 'Falco')
    falco_id = package('Falco', falco, 22)
    from prepare_menu_fonts import disc_file
    for key, disc_name in (('scarlet', 'PlFcRe.dat'), ('forest', 'PlFcGr.dat')):
        raw = output/(key+'-source.dat');raw.write_bytes(disc_file(args.iso.resolve(),disc_name))
        bound = bind_costume(raw,key,falco/'costume.dat')
        result = tool('import-custom-costume',bound,falco_id,key.title())
        identity = re.search(r'ID: ([0-9a-f]{64})',result)[1]
        receipt['costumes'][key] = {'id':identity,'file':str(raw),'title':key.title()}
        (output/'fighters.json').write_text(json.dumps(receipt,indent=2))
    for key, title in (('nook_island', 'Island Shirt'),
                       ('nook_sweater', 'Tom Nook Sweater'),
                       ('cowboy_green', 'Cowboy Green'), ('cowboy_blue', 'Cowboy Blue')):
        model = getattr(args, key)
        if model:
            bound = bind_costume(model.resolve(), key, falco/'costume.dat')
            result = tool('import-custom-costume', bound, falco_id, title)
            identity = re.search(r'ID: ([0-9a-f]{64})', result)[1]
            receipt['costumes'][key] = {'id': identity, 'file': str(model.resolve()), 'title': title}
            (output / 'fighters.json').write_text(json.dumps(receipt, indent=2))
            print(f'{title}: {identity}', flush=True)


if __name__ == '__main__':
    main()
