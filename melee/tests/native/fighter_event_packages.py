"""Exercise named recipient events and immutable bindings in real packages."""
from pathlib import Path
import subprocess
import sys
import tempfile
from fighter_script_bundle_roundtrip import run, portrait, bundled_id

tool, executable, iso = sys.argv[1:]
with tempfile.TemporaryDirectory(prefix='freak-event-packages-') as directory:
    root = Path(directory)
    image, store = root / 'portrait.png', root / 'store'
    portrait(image)
    ids = []
    for index, padding, marker in ((0, 1, 17), (1, 7, 29)):
        project = root / f'fighter-{index}'
        run(tool, 'clone-stock', iso, 'Mario', project, f'Events {index}')
        source = root / f'events-{index}.fms'
        (project / 'fighter_parameters.fmp').write_text(f'int marker = {marker}\n')
        source.write_text(f'state int padding[{padding}]\nstate int received\n'
                          'export func receive():\n    state.received = attrs.marker\n'
                          'action Events:\n    on load:\n        pass\n')
        run(tool, 'script-build', project, source)
        ids.append(bundled_id(run(tool, '--root', store, 'bundle-project', project, image, iso)))
    result = subprocess.run([executable, iso, str(store), *ids], capture_output=True, text=True)
    print(result.stdout + result.stderr, end='')
    raise SystemExit(result.returncode)
