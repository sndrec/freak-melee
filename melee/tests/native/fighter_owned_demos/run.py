"""Exercise real stock construction and independently check owned demo bytes.

All outputs and app data are isolated on A:. This does not run gameplay, compile
fighter behavior or assert result-screen rendering; those are separate gates.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[3]
FIGHTERS = (
    ('Mario', 'Mr'), ('Fox', 'Fx'), ('Captain Falcon', 'Ca'),
    ('Donkey Kong', 'Dk'), ('Bowser', 'Kp'), ('Link', 'Lk'), ('Sheik', 'Sk'),
    ('Ness', 'Ns'), ('Peach', 'Pe'), ('Popo', 'Pp'), ('Nana', 'Nn'),
    ('Pikachu', 'Pk'), ('Samus', 'Ss'), ('Yoshi', 'Ys'), ('Jigglypuff', 'Pr'),
    ('Mewtwo', 'Mt'), ('Luigi', 'Lg'), ('Marth', 'Ms'), ('Zelda', 'Zd'),
    ('Young Link', 'Cl'), ('DrMario', 'Dr'), ('Falco', 'Fc'), ('Pichu', 'Pc'),
    ('Game & Watch', 'Gw'), ('Ganon', 'Gn'), ('Roy', 'Fe'),
)


def sha(data):
    return hashlib.sha256(data).hexdigest()


class Dat:
    def __init__(self, data):
        self.bytes = data
        self.size, self.data_size, relocs, roots, externs = struct.unpack_from('>5I', data)
        assert 32 <= self.size <= len(data)
        table = 32 + self.data_size + relocs * 4
        strings = table + (roots + externs) * 8
        assert strings <= self.size
        self.roots = {}
        for index in range(roots):
            offset, name = struct.unpack_from('>II', data, table + index * 8)
            start = strings + name
            value = data[start:data.index(0, start)].decode('ascii')
            assert value not in self.roots
            self.roots[value] = offset

    def word(self, offset):
        assert 0 <= offset <= self.data_size - 4
        return struct.unpack_from('>I', self.bytes, 32 + offset)[0]

    def text(self, offset):
        assert 0 <= offset < self.data_size
        start = 32 + offset
        return self.bytes[start:self.bytes.index(0, start)].decode('ascii')


def disc_files(iso):
    with iso.open('rb') as source:
        header = source.read(0x440)
        assert header[:6] == b'GALE01' and header[7] == 2
        offset, size = struct.unpack_from('>II', header, 0x424)
        source.seek(offset)
        table = source.read(size)
    count = struct.unpack_from('>I', table, 8)[0]
    names = table[count * 12:]
    files = {}
    for index in range(1, count):
        name, offset, size = struct.unpack_from('>III', table, index * 12)
        if name >> 24:
            continue
        start = name & 0xffffff
        files[names[start:names.index(0, start)].decode('ascii')] = (offset, size)
    return files


def read_disc(iso, files, name):
    offset, size = files[name]
    with iso.open('rb') as source:
        source.seek(offset)
        data = source.read(size)
    assert len(data) == size
    return data


def source_demo_clips(iso, files):
    clips, archives = {}, {}
    for filename in files:
        if not (filename.startswith('GmRstM') or filename.startswith('IrEz') or
                filename in ('IrAls.dat', 'GmRegEnd.dat', 'Vi0102.dat', 'Vi1101.dat') or 'DViWaitA' in filename):
            continue
        data = read_disc(iso, files, filename)
        archive = Dat(data)
        archives[filename] = sha(data)
        for bank_name, root in archive.roots.items():
            if not bank_name.startswith('ftDemo') and filename not in ('Vi0102.dat', 'Vi1101.dat'):
                continue
            cursor, end = 32 + root, 32 + archive.data_size
            while cursor < end:
                if data[cursor] == 255:
                    cursor += 1
                    continue
                if end - cursor < 32:
                    break
                size = struct.unpack_from('>I', data, cursor)[0]
                if size < 32 or size > end - cursor:
                    break
                blob = data[cursor:cursor + size]
                try:
                    clip = Dat(blob)
                except (AssertionError, ValueError, struct.error):
                    # A bank may be followed by unrelated outer-archive data.
                    # Descriptors, not this scan, establish required coverage.
                    break
                for symbol in clip.roots:
                    clips.setdefault(symbol, []).append({
                        'archive': filename, 'bank': bank_name,
                        'archive_offset': cursor, 'bytes': size, 'sha256': sha(blob)})
                cursor += size
    return clips, archives


def inventory(project, original, bank, source_clips, count):
    fighter = Dat((project / 'fighter.dat').read_bytes())
    stock = Dat(original)
    root_name = next(name for name in stock.roots if name.startswith('ftData'))
    root, stock_root = fighter.roots[root_name], stock.roots[root_name]
    demo, original_demo = fighter.word(root + 20), stock.word(stock_root + 20)
    rows = []
    for index in range(count):
        entry, previous = demo + index * 24, original_demo + index * 24
        size = fighter.word(entry + 8)
        assert size == stock.word(previous + 8), index
        # Relocated pointer words may move during Nana's article materialization.
        # The pointee/name, flags and raw command-pointer semantics must agree.
        assert fighter.word(entry + 16) == stock.word(previous + 16), index
        for word in (12, 20):
            assert fighter.word(entry + word) == stock.word(previous + word), index
        if size == 0:
            continue
        symbol = fighter.text(fighter.word(entry))
        assert symbol == stock.text(stock.word(previous))
        offset = fighter.word(entry + 4)
        assert offset % 32 == 0 and offset + size <= len(bank), (index, offset, size)
        blob = bank[offset:offset + size]
        assert symbol in Dat(blob).roots, (index, symbol)
        matches = [source for source in source_clips.get(symbol, [])
                   if source['sha256'] == sha(blob) and source['bytes'] == size]
        assert matches, ('Owned clip differs from all independently read demo sources', index, symbol)
        rows.append({'index': index, 'symbol': symbol, 'offset': offset,
                     'bytes': size, 'sha256': sha(blob), 'disc_matches': matches})
    expected = {index for index in range(count) if stock.word(original_demo + index * 24 + 8)}
    assert {row['index'] for row in rows} == expected
    return {'root': root_name, 'clips': rows}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tool', type=Path, default=ROOT / 'build/native/native_content_tool.exe')
    parser.add_argument('--iso', type=Path, default=ROOT.parent / 'Super Smash Bros. Melee (USA) (v1.02).iso')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output, tool, iso = args.output.resolve(), args.tool.resolve(), args.iso.resolve()
    assert output.drive.upper() == 'A:' and not output.exists(), output
    output.mkdir(parents=True)
    env = os.environ.copy()
    for variable in ('LOCALAPPDATA', 'APPDATA', 'TEMP', 'TMP'):
        directory = output / variable.lower()
        directory.mkdir()
        env[variable] = str(directory)
    files = disc_files(iso)
    clips, archive_hashes = source_demo_clips(iso, files)
    pinned = {str(path): sha(path.read_bytes()) for path in (
        Path(__file__), tool, ROOT / 'src/melee/ft/native_fighter_project.c')}
    receipt = {'status': 'RUNNING', 'tool_inputs': pinned, 'disc_archives': archive_hashes,
               'cases': {}, 'limits': ['Byte provenance and construction only; no gameplay/render/behavior compilation.',
                                      'No stock Kirby clone requested; no single-player modes activated.'],
               'isolation': 'Fresh A: projects/store/app-data; CREATE_NO_WINDOW; no games.'}

    def save():
        (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')

    try:
        for name, short in FIGHTERS:
            directory = output / short
            directory.mkdir()
            project = directory / 'project'
            command = [str(tool), '--root', str(output / 'store'), 'clone-stock',
                       str(iso), name, str(project), 'Owned demo ' + name]
            result = subprocess.run(command, env=env, cwd=ROOT, capture_output=True,
                                    encoding='utf-8', errors='replace', timeout=120,
                                    creationflags=subprocess.CREATE_NO_WINDOW)
            (directory / 'clone.log').write_text(result.stdout + result.stderr, encoding='utf-8')
            assert result.returncode == 0, (name, result.returncode, result.stdout, result.stderr)
            original = read_disc(iso, files, 'Pl' + short + '.dat')
            bank = (project / 'animation.dat').read_bytes()
            case = inventory(project, original, bank, clips, 16 if name in ('Mario', 'Luigi') else 14)
            case.update(command=command, exit=result.returncode,
                        files={path.name: sha(path.read_bytes()) for path in project.iterdir() if path.is_file()})
            receipt['cases'][name] = case
            save()
            print(name + ': ' + str(len(case['clips'])) + ' independently owned demo clips PASS', flush=True)
        assert all(sha(Path(path).read_bytes()) == digest for path, digest in pinned.items())
        total = sum(len(case['clips']) for case in receipt['cases'].values())
        assert total == 343
        receipt['status'] = 'PASS all 26 requested stock clone constructors / 343 owned demo clips'
    except Exception as error:
        receipt.update(status='FAIL', error=repr(error))
        save()
        raise
    save()


if __name__ == '__main__':
    main()
