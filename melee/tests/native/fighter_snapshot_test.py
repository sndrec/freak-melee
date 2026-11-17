"""Exercise whole-project saves with real single and paired stock assets."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

tool, probe, iso = map(lambda p: str(Path(p).resolve()), sys.argv[1:])
windows = Path(os.environ['SystemRoot'])
env = dict(os.environ, PATH=str(windows / 'System32') + ';' + str(windows))
with tempfile.TemporaryDirectory(prefix='native-snapshot-') as temp:
    for fighter in ('Mario', 'Zelda'):
        project = str(Path(temp) / fighter)
        for command in ([tool, 'clone-stock', iso, fighter, project, fighter], [probe, project]):
            result = subprocess.run(command, capture_output=True, text=True, env=env, timeout=90,
                                    creationflags=subprocess.CREATE_NO_WINDOW)
            assert result.returncode == 0, (command, result.stdout, result.stderr)
            if command[0] == probe: print(result.stdout.strip())
