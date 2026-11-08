"""Build independent fighters and outfits for the startup movie.

Uses the ordinary Workshop authoring APIs through native_content_tool. All
generated DATs and installed packages stay in the chosen production directory.
The three optional costume DATs are supplied by the filmmaker; credits live in
docs/native_intro_production.md.
"""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('tool', 'iso', 'output', 'portraits'):
        parser.add_argument('--' + name, type=Path, required=True)
    for name in ('rude-falco', 'nook-island', 'nook-sweater'):
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
        tool('project-resource-import', project, 'ui/player-card.png', 'png',
             args.portraits.resolve() / f'card-{portrait}.png')
        tool('script-build', project, project / 'behavior.fms')
        result = tool('package-project', project, project / 'portrait.png')
        identity = re.search(r'\b[0-9a-f]{64}\b', result)[0]
        tool('install-project', project, identity)
        receipt['fighters'][key] = {'id': identity, 'project': str(project)}
        (output / 'fighters.json').write_text(json.dumps(receipt, indent=2))
        print(f'{key}: {identity}', flush=True)
        return identity

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
                        '            velocity.y = velocity.y + 0.04')
    source.write_text(text)
    values = edited / 'fighter_parameters.fmp'
    text = values.read_text()
    assert 'float It_Kind_Mario_Fire.launch_speed = 1.5' in text
    values.write_text(text.replace('float It_Kind_Mario_Fire.launch_speed = 1.5',
                                   'float It_Kind_Mario_Fire.launch_speed = 3.2'))
    package('MarioRemix', edited, 0)
    ganon = clone('Ganondorf', 'Ganondorf', 'Ganondorf')
    tool('project-hitbox-power', ganon, 1, 100, 400)
    package('Ganondorf', ganon, 25)
    falco = clone('Falco', 'Falco', 'Falco')
    falco_id = package('Falco', falco, 20)
    for key, title in (('rude_falco', 'Rude Falco'), ('nook_island', 'Island Shirt'),
                       ('nook_sweater', 'Tom Nook Sweater')):
        model = getattr(args, key)
        if model:
            result = tool('import-custom-costume', model.resolve(), falco_id, title)
            identity = re.search(r'ID: ([0-9a-f]{64})', result)[1]
            receipt['costumes'][key] = {'id': identity, 'file': str(model.resolve()), 'title': title}
            (output / 'fighters.json').write_text(json.dumps(receipt, indent=2))
            print(f'{title}: {identity}', flush=True)


if __name__ == '__main__':
    main()
