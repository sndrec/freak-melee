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
            process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, env=env,
                                       creationflags=subprocess.CREATE_NO_WINDOW)
            stdout, stderr = process.communicate(timeout=90)
            assert process.returncode == 0, (command, stdout, stderr)
            if command[0] == probe:
                assert not list(Path(tempfile.gettempdir()).glob(f'FreakMeleeValidation-{process.pid}-*')), 'Validation left temporary asset copies'
                print(stdout.strip())
