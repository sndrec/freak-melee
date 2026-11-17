"""Review source-only families, edited assets, room caching and recipient editing.

Uses fresh copies of a fixture profile, hidden/muted games and a caller-supplied
local signaling service. No public chat or real installed content is changed.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import secrets
import struct
import subprocess
import time

from PIL import Image
from review_native_css_room import Client


CHUNKS = ('fighter.dat', 'animation.dat', 'costume.dat', 'assets.fma',
          'behavior.fms', 'fighter_parameters.fmp', 'cpu_profile.fcp', 'resources.fmr')
CASES = [('zelda', 19, 1, 2), ('climbers', 10, 2, 2), ('fox', 1, 0, 1)]
MEMBERS = {'zelda': ['Zelda', 'Sheik'], 'climbers': ['Popo', 'Nana'], 'fox': ['Fox']}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def package(path):
    data = path.read_bytes()
    assert data[:5] == b'FMSC\0' and data[7] == 0
    mode, count = data[5:7]
    assert (mode, count) in [(0, 1), (1, 2), (2, 2)]
    portrait_bytes = struct.unpack_from('<I', data, 8)[0]
    tile_bytes = int.from_bytes(data[12:16], 'little')
    assert tile_bytes < 64
    at = 208 + portrait_bytes + tile_bytes
    members = []
    for member in range(count):
        row = 80 + member * 64
        key = data[row:row+32].split(b'\0', 1)[0].decode('ascii')
        sizes = struct.unpack_from('<8I', data, row+32)
        chunks = {}
        for name, size in zip(CHUNKS, sizes):
            assert at + size <= len(data)
            chunks[name] = data[at:at+size]
            at += size
        assert chunks['behavior.fms'] and chunks['assets.fma']
        members.append({'key': key, 'chunks': chunks})
    assert at == len(data)
    return {'mode': mode, 'count': count, 'members': members,
            'title': data[16:80].split(b'\0', 1)[0].decode('utf-8'),
            'id': digest(data)}


def wait(predicate, label, timeout=60):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        value = predicate()
        if value:
            return value
        time.sleep(.15)
    raise RuntimeError('Timed out: ' + label)


def regions(client, name):
    text = client.inspect('hub-regions')
    return {int(value): (round(float(x)+float(w)/2), round(float(y)+float(h)/2))
            for x, y, w, h, value in re.findall(
                rf'\[hub-inspect\] {re.escape(name)} origin=([\d.-]+),([\d.-]+) size=([\d.-]+),([\d.-]+) enabled=1 value=(\d+)', text)}


def choose(client, identifier=None):
    client.screenshot()
    layout = (client.data / 'css-layout.bin').read_bytes()
    count = struct.unpack_from('<H', layout, 4)[0]
    tile = next(i for i in range(count)
                if identifier is None or layout[14+48*i:46+48*i].hex() == identifier)
    state, text = client.state()
    x, y = map(float, re.search(rf'\[css-tile\] index={tile} center=([\d.-]+),([\d.-]+)', text).groups())
    if state['hand'] != 1:
        client.tap('b')
    client.move(x-2.7, y+2)
    client.tap('a')
    assert client.state()[0]['holding'] == 0 and client.state()[0]['chosen'] == 1


class Review:
    def __init__(self, args):
        self.args = args
        self.output = args.output.resolve()
        self.output.mkdir(parents=True, exist_ok=False)
        self.clients = []
        mode = 'recipient authoring/live' if args.recipient_profile else 'prepare' if args.prepare_only else 'full integration'
        self.receipt = {'status': 'RUNNING', 'mode': mode, 'cases': [], 'commands': [], 'inputs': {}}
        for path in [Path(__file__).resolve(), args.exe.resolve(), args.tool.resolve()]:
            with path.open('rb') as file:
                self.receipt['inputs'][str(path)] = hashlib.file_digest(file, 'sha256').hexdigest()
        self.save()

    def save(self):
        (self.output / 'receipt.json').write_text(json.dumps(self.receipt, indent=2), encoding='utf-8')

    def client(self, profile, folder, name, **options):
        c = Client(self.args.exe.resolve(), self.args.iso.resolve(), profile,
                   self.output / folder, name, window_size=(960, 540), **options)
        self.clients.append(c)
        return c

    def cli(self, client, label, *args):
        env = dict(os.environ, LOCALAPPDATA=str(client.data.parent),
                   PATH=os.path.join(os.environ['SystemRoot'], 'System32'))
        command = [str(self.args.tool.resolve()), '--root', str(client.data/'content'), *map(str, args)]
        result = subprocess.run(command, cwd=client.folder, env=env, capture_output=True,
                                encoding='utf-8', errors='replace', timeout=180,
                                creationflags=subprocess.CREATE_NO_WINDOW)
        (self.output / (label+'.log')).write_text(result.stdout+result.stderr, encoding='utf-8')
        self.receipt['commands'].append({'label': label, 'argv': command, 'exit': result.returncode})
        self.save()
        assert result.returncode == 0, (label, result.stdout, result.stderr)
        return result.stdout

    def fixture(self):
        creator = self.client(self.args.profile.resolve(), 'creator', 'Content Author')
        for label, kind, mode, count in CASES:
            title = 'Review ' + label
            creator.click('clone-fighter')
            targets = regions(creator, 'template')
            assert kind in targets, targets
            creator.command('ui-click', *targets[kind])
            creator.enter('clone-name', title)
            creator.click('create-clone'); creator.element('edit-fighter')
            def created():
                for draft in (creator.data/'projects').glob('fighter-*'):
                    if (draft/'installed.id').exists() and ('title='+title+'\n') in (draft/'project.txt').read_text():
                        return draft
            draft = wait(created, 'create source-only ' + label)
            original = package(creator.data/'content/cache'/((draft/'installed.id').read_text().strip()+'.fmc'))
            assert (original['mode'], original['count']) == (mode, count), original
            self.receipt['cases'].append({'label': label, 'title': title, 'mode': mode,
                                          'count': count, 'draft': str(draft), 'original_id': original['id']})
            creator.screenshot()
        creator.stop()
        # Each member gets an edited texture, copied clip and a source-owned
        # item/PNG pair. The new neutral-B line exercises that resource in play.
        for case in self.receipt['cases']:
            root = Path(case['draft'])
            changes = []
            for member in range(case['count']):
                draft = root if member == 0 else root/'partner'
                label = f"{case['label']}-{member}"
                before = {name: digest((draft/name).read_bytes()) if (draft/name).exists() else None for name in CHUNKS}
                image = self.output / (label+'.png')
                pixels = Image.new('RGBA', (32, 32))
                for y in range(32):
                    for x in range(32):
                        pixels.putpixel((x, y), ((255, 60+member*100, 100, 255) if (x//8+y//8)%2 else (40, 70, 255, 255)))
                pixels.save(image)
                self.cli(creator, label+'-texture', 'project-model-texture', draft, 0, image)
                actions = self.cli(creator, label+'-actions', 'project-actions', draft)
                names = {name: int(index) for index, name in re.findall(r'^(\d+) (\S+)$', actions, re.M)}
                source = next(index for name, index in names.items() if name.startswith('AttackAirN'))
                target = next(index for name, index in names.items() if name.startswith('AttackAirB'))
                self.cli(creator, label+'-clip', 'project-clip-copy', draft, 'action', source, target, 'ReviewCopiedAir')
                blueprint=self.output/(label+'.item')
                self.cli(creator,label+'-blueprint','agent','call','resources.blueprint',json.dumps(dict(
                    iso=str(self.args.iso.resolve()),stock_kind=0,output=str(blueprint))))
                family_root=root
                selection=dict(project=str(family_root),member='partner' if member else 'main')
                revision=json.loads(self.cli(creator,label+'-resources','agent','call','resources.read',json.dumps(selection)))['result']['revision']
                self.cli(creator,label+'-resource-edit','agent','call','resources.edit',json.dumps(dict(selection,revision=revision,changes=[
                    dict(operation='import',name='review/token.png',kind='png',path=str(image)),
                    dict(operation='import',name='review/token.item',kind='blob',path=str(blueprint))])))
                source_path = draft/'behavior.fms'
                text = source_path.read_text(encoding='utf-8')
                assert not re.search(r'^item ', text, re.M) and '    on special_n:\n' in text
                text = text.replace('    on special_n:\n', '    on special_n:\n        items.pull(0)\n', 1)
                text += ('\nitem ReviewToken damage 4 angle 45 lifetime 180:\n'
                         '    blueprint review/token.item\n    icon review/token.png\n'
                         '    billboard_size 6\n    on thrown:\n        velocity.y = velocity.y + 1\n')
                source_path.write_text(text, encoding='utf-8', newline='\n')
                self.cli(creator, label+'-source', 'script-build', draft, source_path)
                after = {name: digest((draft/name).read_bytes()) if (draft/name).exists() else None for name in CHUNKS}
                assert all(before[name] != after[name] for name in ['costume.dat', 'animation.dat', 'behavior.fms', 'resources.fmr']), (label, before, after)
                changes.append({'member': member, 'before': before, 'after': after})
            built = self.cli(creator, case['label']+'-package', 'package-project', root, root/'portrait.png')
            identifier = re.search(r'Cached source fighter ([0-9a-f]{64})', built)[1]
            self.cli(creator, case['label']+'-verify', 'verify-package', identifier)
            self.cli(creator, case['label']+'-install', 'install-project', root, identifier)
            current = package(creator.data/'content/cache'/(identifier+'.fmc'))
            assert (current['mode'], current['count']) == (case['mode'], case['count'])
            assert current['id'] == identifier != case['original_id']
            for member, payload in enumerate(current['members']):
                assert all(digest(data) == changes[member]['after'][name] for name, data in payload['chunks'].items())
            case.update(id=identifier, edits=changes)
            self.save()
            print(case['label'] + ': source family and edited member assets prepared', flush=True)
        self.receipt['creator_data'] = str(creator.data)
        return creator.data

    def select_catalog(self, client, identifier):
        listing = self.cli(client, 'catalog-'+secrets.token_hex(3), 'list')
        ids = re.findall(r'^([0-9a-f]{64})  .*  fighter family \(', listing, re.M)
        index = ids.index(identifier)
        targets = regions(client, 'fighter')
        assert index in targets, (ids, targets)
        client.command('ui-click', *targets[index])
        time.sleep(.2)

    def transfer(self, creator_data):
        sender = self.client(creator_data.parent, 'sender', 'Family Sender')
        receiver = self.client(self.args.profile.resolve(), 'receiver', 'Family Receiver', without_drafts=True)
        room = ''.join(secrets.choice('ABCDEFGHJKLMNPQRSTUVWXYZ23456789') for _ in range(10))
        self.receipt['room'] = room
        for i, c in enumerate([sender, receiver]):
            c.click('nav-1'); c.enter('lobby-code', room); c.click('join-lobby'); c.wait_cards(i+1)
            c.click('lobby-ready'); c.screenshot()
        receiver.wait_cards(2); sender.wait_cards(2)
        choose(receiver)
        installed_before = set((receiver.data/'content/installed').glob('*.fmi'))
        for case in self.receipt['cases']:
            identifier = case['id']
            choose(sender, identifier)
            marker = receiver.data/'content/installed'/(identifier+'.fmi')
            cached = receiver.data/'content/cache'/(identifier+'.fmc')
            roster = (receiver.data/'css-layout.bin').read_bytes()
            assert not marker.exists()
            wait(lambda: cached.exists() and 'receiving=0' in receiver.inspect('css-cards'), 'auto-cache '+case['label'], 120)
            assert digest(cached.read_bytes()) == identifier
            cards = wait(lambda: (s if re.search(r'\[css-card\].*local=0.*selected=1.*portrait=1.*title='+re.escape(case['title']), s := receiver.inspect('css-cards')) else None), 'remote portrait')
            assert not marker.exists() and (receiver.data/'css-layout.bin').read_bytes() == roster
            receiver.screenshot()
            for c in [sender, receiver]:
                c.tap('b', 40); c.click('nav-5')
            self.select_catalog(sender, identifier)
            sender.click('send-fighter'); sender.click('gift-recipient'); sender.click('confirm-gift')
            receiver.click('accept-fighter')
            wait(lambda: marker.exists() and (receiver.data/'css-layout.bin').read_bytes() != roster,
                 'install and roster publication '+case['label'])
            assert (receiver.data/'css-layout.bin').read_bytes() != roster
            assert digest(cached.read_bytes()) == identifier
            # A repeated cached delivery must retain one installation/tile.
            roster_after = (receiver.data/'css-layout.bin').read_bytes()
            sender.click('send-fighter'); sender.click('gift-recipient'); sender.click('confirm-gift')
            receiver.click('accept-fighter'); time.sleep(.4)
            assert (receiver.data/'css-layout.bin').read_bytes() == roster_after
            case['delivery'] = {'auto_cache': True, 'explicit_install': True, 'duplicate_preserved': True, 'cards': cards}
            receiver.screenshot()
            for c in [sender, receiver]:
                c.click('local-practice'); c.wait_cards(2)
            self.save()
            print(case['label'] + ': automatic cache, explicit gift and duplicate protection passed', flush=True)
        assert len(list((receiver.data/'content/installed').glob('*.fmi'))) == len(installed_before)+3
        sender.stop()
        for data in [creator_data, sender.data]:
            projects = (data/'projects').resolve()
            assert projects.is_relative_to(data.parent.parent.resolve())
            if projects.exists():
                projects.rename(data/('author-projects-offline-'+self.output.name))
        assert not list((receiver.data/'projects').glob('fighter-*'))
        receiver.tap('b', 40); receiver.click('nav-1'); receiver.click('leave-lobby'); receiver.click('nav-5')
        self.recipient(receiver)

    def recipient(self, receiver):
        receiver.resize(1600, 1000)
        receiver.screenshot()
        for case in self.receipt['cases']:
            self.select_catalog(receiver, case['id'])
            receiver.click('edit-fighter'); receiver.element('workshop-editor-header')
            wait(lambda: 'text=Draft preview ready.' in receiver.inspect('workshop-editor-status'), 'received preview '+case['label'])
            options = re.findall(r'\[ui-inspect-option\] workshop-family-member index=\d+ value=(\d+) text=([^\r\n]*)',
                                 receiver.inspect('workshop-family-member'))
            assert options == [(str(i), name) for i, name in enumerate(MEMBERS[case['label']])], options
            if case is self.receipt['cases'][0]:
                self.header(receiver)
            draft = next(p for p in (receiver.data/'projects').glob('fighter-*')
                         if (p/'installed.id').read_text().strip() == case['id'])
            packet = package(receiver.data/'content/cache'/(case['id']+'.fmc'))
            for member, payload in enumerate(packet['members']):
                directory = draft if not member else draft/'partner'
                assert all((directory/name).read_bytes() == data for name, data in payload['chunks'].items()), (case['label'], member)
                if member:
                    receiver.click('workshop-family-member'); receiver.command('ui-key', 'down'); receiver.command('ui-key', 'enter')
                    wait(lambda: 'value=1' in receiver.inspect('workshop-family-member'), 'select partner')
                    wait(lambda: 'text=Draft preview ready.' in receiver.inspect('workshop-editor-status'), 'partner preview')
                receiver.click('workshop-mode-script'); receiver.click('workshop-script-validate')
                wait(lambda: 'text=Source and fighter assets validated.' in receiver.inspect('workshop-script-status'), 'source closure '+case['label'])
                case.setdefault('frozen_members', []).append(self.frozen(receiver, case, member))
                receiver.screenshot()
                self.cli(receiver, case['label']+f'-received-{member}', 'source-check', directory, directory/'behavior.fms')
            rebuilt = self.cli(receiver, case['label']+'-recipient-package', 'package-project', draft, draft/'portrait.png')
            # Rebuilding an untouched received draft preserves its complete ID.
            assert re.search(r'Cached source fighter ([0-9a-f]{64})', rebuilt)[1] == case['id'], rebuilt
            receiver.click('workshop-test-character')
            receiver.element('workshop-return-editor')
            time.sleep(4)
            # This fixture pulls unconditionally into an empty hand. One press
            # exercises its owned item; another would fail with an occupied hand.
            receiver.tap('b', frames=2)
            time.sleep(1)
            image = receiver.screenshot()
            assert receiver.process.poll() is None
            errors = re.findall(r'\[script\] Fighter .* event .* source .*',
                                (receiver.folder/'stdout.log').read_text(errors='replace'))
            assert not errors, errors
            live = self.preview(receiver, case['count'])
            assert live['live'] == 1
            active = [m for m in live['members'] if not m['dormant']]
            assert len(active) == (2 if case['mode'] == 2 else 1), live
            assert all(m['visible'] and m['paused'] < m['procs'] for m in active), live
            case['live_members'] = live
            receiver.click('workshop-return-editor')
            wait(lambda: 'text=Draft preview ready.' in receiver.inspect('workshop-editor-status'), 'return from received live test')
            case['returned_preview'] = self.frozen(receiver, case, case['count']-1)
            case['live_test_capture'] = str(image)
            receiver.click('workshop-back-drafts'); receiver.element('edit-fighter')
            case['received_draft'] = str(draft)
            case['recipient_members_validated'] = case['count']
            self.save()
            print(case['label'] + ': every received member opens and validates with author offline', flush=True)

    def preview(self, client, count):
        log = client.folder/'stdout.log'
        start = log.stat().st_size
        client.command('ui-inspect', 'workshop-preview')
        def sample():
            with log.open(errors='replace') as file:
                file.seek(start)
                text = file.read()
            header = re.search(r'\[workshop-preview\] editing=(\d+) live=(\d+) transitioning=(\d+) selected=(\d+)', text)
            rows = re.findall(r'\[workshop-member\] member=(\d+) id=(\d+) kind=(\d+) dormant=(\d+) visible=(\d+) procs=(\d+) paused=(\d+) frame=([^ ]+) pos=([^\r\n]+)', text)
            if not header or len(rows) != count:
                return None
            result = dict(zip(['editing', 'live', 'transitioning', 'selected'], map(int, header.groups())))
            result['members'] = [dict(zip(['member', 'id', 'kind', 'dormant', 'visible', 'procs', 'paused'], map(int, row[:7])),
                                      frame=row[7], position=row[8]) for row in rows]
            return result
        return wait(sample, 'family preview state', 5)

    def frozen(self, client, case, member):
        first = self.preview(client, case['count'])
        assert first['editing'] and not first['live'] and not first['transitioning'] and first['selected'] == member, first
        assert [m['member'] for m in first['members'] if m['visible']] == [member], first
        assert all(m['dormant'] or (m['procs'] > 0 and m['paused'] == m['procs']) for m in first['members']), first
        time.sleep(.7)
        second = self.preview(client, case['count'])
        assert first == second, (first, second)
        return first

    def header(self, client):
        modes = [('action', 'workshop-action-viewport'), ('script', 'workshop-script-pane'),
                 ('animation', 'workshop-animation-pane'), ('model', 'workshop-model-pane')]
        controls = ['workshop-back-drafts', 'workshop-family-member',
                    *('workshop-mode-'+mode for mode, _ in modes),
                    'workshop-undo', 'workshop-redo', 'workshop-package-toggle',
                    'workshop-save-top', 'workshop-test-character']
        captures = []
        for width, height in [(960, 600), (1280, 800), (1440, 900), (1600, 1000)]:
            client.resize(width, height)
            client.screenshot()
            rects = {}
            for name in controls:
                inspection = client.inspect(name)
                assert 'visible=1' in inspection, inspection
                rect = tuple(map(float, re.search(r'origin=([\d.-]+),([\d.-]+) size=([\d.-]+),([\d.-]+)', inspection).groups()))
                x, y, w, h = rect
                assert x >= 0 and y >= 0 and x+w <= client.size[0]+1 and w > 0 and h > 0, (name, rect, client.size)
                for other, (ox, oy, ow, oh) in rects.items():
                    assert min(x+w, ox+ow)-max(x, ox) <= 1 or min(y+h, oy+oh)-max(y, oy) <= 1, (name, other, rect, rects[other])
                rects[name] = rect
            for mode, panel in modes:
                client.click('workshop-mode-'+mode)
                assert 'visible=1' in client.inspect(panel), (width, mode)
                captures.append(str(client.screenshot()))
        client.click('workshop-mode-action')
        self.receipt['header_captures'] = captures
        self.save()

    def run(self):
        try:
            if self.args.prepared:
                prior = json.loads(self.args.prepared.read_text())
                creator = Path(prior['creator_data']).resolve()
                assert creator.is_relative_to(self.args.prepared.resolve().parent)
                assert len(prior['cases']) == 3 and all('id' in case for case in prior['cases'])
                self.receipt.update(cases=prior['cases'], creator_data=str(creator), prepared=str(self.args.prepared.resolve()))
                self.save()
            else:
                creator = self.fixture()
            if self.args.recipient_profile:
                assert not (creator/'projects').exists(), 'Retire the author project paths before this check'
                receiver = self.client(self.args.recipient_profile.resolve(), 'receiver', 'Received Authoring')
                self.recipient(receiver)
            elif not self.args.prepare_only:
                self.transfer(creator)
            self.receipt['status'] = 'PASS ' + self.receipt['mode']
        except Exception as error:
            self.receipt.update(status='FAIL', error=repr(error))
            for c in self.clients:
                if c.process.poll() is None:
                    try:
                        c.screenshot()
                    except Exception as capture_error:
                        print('Could not capture failure: '+repr(capture_error), flush=True)
            raise
        finally:
            self.save()
            for c in reversed(self.clients):
                c.stop()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'iso', 'profile', 'output']:
        p.add_argument('--'+name, type=Path, required=True)
    p.add_argument('--tool', type=Path)
    p.add_argument('--endpoint', required=True)
    p.add_argument('--prepare-only', action='store_true')
    p.add_argument('--prepared', type=Path, help='Reuse the completed fixtures in this receipt')
    p.add_argument('--recipient-profile', type=Path, help='Review already received content without repeating transfers')
    args = p.parse_args()
    if args.tool is None:
        args.tool = args.exe.with_name('native_content_tool.exe')
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = args.endpoint
    Review(args).run()


if __name__ == '__main__':
    main()
