"""Check article isolation through actual registered copied fighter packages."""
from pathlib import Path
import subprocess
import sys
import tempfile
from fighter_script_bundle_roundtrip import run, portrait, bundled_id

tool, executable, iso = sys.argv[1:]
with tempfile.TemporaryDirectory(prefix='freak-article-ownership-') as directory:
    root = Path(directory)
    image, store = root / 'portrait.png', root / 'store'
    portrait(image)
    ids = []
    for index, name in enumerate(('Mario', 'Mario', 'DrMario')):
        project = root / f'fighter-{index}'
        run(tool, 'clone-stock', iso, name, project, f'Ownership {index}')
        ids.append(bundled_id(run(tool, '--root', store, 'bundle-project', project, image, iso)))
    result = subprocess.run([executable, iso, str(store), *ids], capture_output=True, text=True)
    print(result.stdout + result.stderr, end='')
    raise SystemExit(result.returncode)
