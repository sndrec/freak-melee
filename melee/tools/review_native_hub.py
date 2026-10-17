"""Exercise the native hub against an isolated local installation.

Runs one hidden, muted client. The control channel supplies UI/controller input;
the user's projects, installed collection and controller profiles remain intact.
Screenshots are optional because entering the Workshop can build GPU pipelines.
"""
import argparse
import os
from pathlib import Path
import re
import struct
import subprocess
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--iso', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--screenshots', action='store_true')
    parser.add_argument('--control', type=Path,
                        help='Development control tool outside a prepared installation')
    parser.add_argument('--script-check-build', action='store_true')
    args = parser.parse_args()
    exe, iso, output = args.exe.resolve(), args.iso.resolve(), args.output.resolve()
    if output.exists():
        raise SystemExit('Choose a new output directory for an isolated first launch.')
    output.mkdir(parents=True)
    appdata = output / 'appdata'
    appdata.mkdir()
    data = appdata / 'Freak Melee'
    control = args.control.resolve() if args.control else exe.with_name('melee_native_control.exe')
    identity = f'hub-review-{os.getpid()}'
    env = dict(os.environ, LOCALAPPDATA=str(appdata), FREAK_MELEE_CONTROL_ID=identity)
    # These tests only exercise local authoring. Keep them out of public chat.
    env['FREAK_MELEE_CHAT_ENDPOINT'] = 'http://127.0.0.1:1'
    stdout, stderr = output / 'stdout.log', output / 'stderr.log'
    flags = subprocess.CREATE_NO_WINDOW

    def command(*parts):
        subprocess.run([str(control), '--id', identity, *map(str, parts)],
                       check=True, capture_output=True, creationflags=flags)

    def installed_count():
        return len(list((data / 'content/installed').glob('*.fmi')))

    def inspect(element, timeout=20, bounds=False):
        deadline = time.monotonic() + timeout
        expression = re.compile(
            r'\[(hub-inspect|ui-inspect)\] ' + re.escape(element) +
            r':?[^\n]*origin=([\d.-]+),([\d.-]+) size=([\d.-]+),([\d.-]+)')
        while time.monotonic() < deadline:
            if client.poll() is not None:
                raise RuntimeError(f'Native client exited with {client.returncode}')
            start = stderr.stat().st_size
            command('ui-inspect', element)
            for _ in range(10):
                time.sleep(.1)
                with stderr.open('r', encoding='utf-8', errors='replace') as log:
                    log.seek(start)
                    text = log.read()
                match = expression.search(text)
                if match:
                    x, y, w, h = map(float, match.groups()[1:])
                    if w > 0 and h > 0 and 'visible=0' not in match[0]:
                        if bounds:
                            return x, y, w, h
                        return round(x + w / 2), round(y + h / 2)
                if '[hub-inspect]' in text or '[ui-inspect]' in text:
                    break
            time.sleep(.1)
        raise RuntimeError(f'{element} did not become available; see {stderr}')

    def click(element):
        command('ui-click', *inspect(element))
        time.sleep(.25)

    def enter(element, text):
        click(element)
        command('ui-key', 'a', 'ctrl')
        time.sleep(.1)
        command('ui-text', text)
        time.sleep(.15)

    def wait_for(predicate, description, timeout=20):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if predicate():
                return
            if client.poll() is not None:
                raise RuntimeError(f'Native client exited with {client.returncode}')
            time.sleep(.1)
        raise RuntimeError(f'Timed out: {description}; see {stderr}')

    def editor_ready():
        start = stderr.stat().st_size
        command('ui-inspect', 'workshop-editor-status')
        time.sleep(.15)
        with stderr.open('r', encoding='utf-8', errors='replace') as log:
            log.seek(start)
            return 'text=Draft preview ready.' in log.read()

    with stdout.open('w') as out, stderr.open('w') as err:
        client = subprocess.Popen([str(exe), '--iso', str(iso), '--hidden', '--mute'],
                                  cwd=output, env=env, stdout=out, stderr=err,
                                  creationflags=flags)
        (output / 'control-id.txt').write_text(identity)
        (output / 'pid.txt').write_text(str(client.pid))
        try:
            wait_for(lambda: 'Native control channel ready' in stdout.read_text(
                errors='replace'), 'control channel')
            inspect('clone-fighter', timeout=60)
            layout = data / 'css-layout.bin'
            assert struct.unpack_from('<H', layout.read_bytes(), 4)[0] == 0
            assert installed_count() == 0
            if args.screenshots:
                command('screenshot')

            click('clone-fighter')
            enter('clone-name', 'Hub Review Mario')
            click('create-clone')
            inspect('edit-fighter')
            wait_for(lambda: installed_count() == 1, 'playable starter clone')
            drafts = list((data / 'projects').glob('fighter-*'))
            assert len(drafts) == 1
            draft = drafts[0]
            assert (draft / 'behavior.fms').stat().st_size > 1000
            assert (draft / 'cpu_profile.fcp').stat().st_size > 0
            assert (draft / 'portrait.png').stat().st_size > 0
            assert struct.unpack_from('<H', layout.read_bytes(), 4)[0] == 1
            original_id = (draft / 'installed.id').read_text()

            click('rename-fighter')
            enter('fighter-title', 'Hub Review Renamed')
            click('apply-name')
            wait_for(lambda: (draft / 'installed.id').read_text() != original_id,
                     'updated installed identity')
            assert installed_count() == 1
            assert 'title=Hub Review Renamed\n' in (draft / 'project.txt').read_text()
            click('copy-fighter')
            wait_for(lambda: installed_count() == 2, 'independent fighter copy')
            assert len(list((data / 'projects').glob('fighter-*'))) == 2
            if args.screenshots:
                command('screenshot')

            # Imported source may have Windows line endings. Viewing it must
            # preserve the file and avoid a false unsaved-edit warning.
            source_path = draft / 'behavior.fms'
            windows_source = source_path.read_bytes().replace(b'\r\n', b'\n').replace(b'\n', b'\r\n')
            source_path.write_bytes(windows_source)
            click('edit-fighter')
            inspect('workshop-editor-header', timeout=60)
            wait_for(editor_ready, 'frozen fighter preview', timeout=60)
            assert source_path.read_bytes() == windows_source
            if args.script_check_build:
                def script_status(expected, element='workshop-script-status'):
                    start = stderr.stat().st_size
                    command('ui-inspect', element)
                    time.sleep(.15)
                    with stderr.open('r', errors='replace') as log:
                        log.seek(start)
                        return expected in log.read()
                click('workshop-mode-script')
                click('workshop-script-validate')
                wait_for(lambda: script_status('Source and fighter assets validated.'), 'packaged script check')
                click('workshop-script-build')
                wait_for(lambda: script_status('Source validated and saved to this draft.'), 'packaged script build')
                click('workshop-package-toggle')
                x, y, w, h = inspect('workshop-inspector', bounds=True)
                # Inspection returns the content box. The scrollbar occupies
                # the rail's right padding; drag its thumb to the bottom.
                command('ui-drag', round(x+w+16), round(y-15), round(x+w+16), round(y+h+10))
                click('workshop-install')
                wait_for(lambda: script_status('text=Installed ', 'workshop-install-status'),
                         'packaged fighter installation')
                assert installed_count() == 2
                installed_id = (draft/'installed.id').read_text().strip()
                assert (data/'content/installed'/f'{installed_id}.fmi').is_file()
            expected_source = source_path.read_bytes()
            if args.screenshots:
                command('screenshot')
            click('workshop-back-drafts')
            inspect('edit-fighter', timeout=60)
            assert source_path.read_bytes() == expected_source

            click('nav-6')
            enter('display-name', 'Local Hub Reviewer')
            click('save-profile')
            wait_for(lambda: (data / 'content/hub-profile.txt').exists(), 'profile save')
            assert (data / 'content/hub-profile.txt').read_text().splitlines()[0] == 'Local Hub Reviewer'
            click('nav-5')
            click('local-practice')
            command('tap', 0, 'b', 'frames=40')
            inspect('clone-fighter')
            if args.screenshots:
                command('screenshot')
                time.sleep(.5)
            (output / 'result.txt').write_text(
                'PASS: empty first launch, playable clone, CSS placement, rename replacement, '
                'independent copy, CRLF editor return, profile save, controller return.\n')
            if args.script_check_build:
                with (output/'result.txt').open('a') as result:
                    result.write('PASS: packaged Workshop source check, behavior build and fighter install.\n')
            print((output / 'result.txt').read_text().strip())
        finally:
            # Only this test's child is stopped. It owns this isolated profile.
            if client.poll() is None:
                client.terminate()
                client.wait(timeout=15)


if __name__ == '__main__':
    main()
