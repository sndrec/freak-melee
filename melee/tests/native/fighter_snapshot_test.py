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
    (Path(temp) / 'userdata').mkdir()
    env['LOCALAPPDATA'] = str(Path(temp) / 'userdata')
    for fighter in ('Mario', 'Zelda'):
        project = str(Path(temp) / fighter)
        commands = [[tool, 'clone-stock', iso, fighter, project, fighter]]
        if fighter == 'Zelda':
            commands.append([tool, 'clone-stock', iso, 'Sheik', str(Path(project) / 'partner'), 'Sheik'])
        commands.append([probe, project])
        for command in commands:
            if command[0] == probe and fighter == 'Zelda':
                (Path(project) / 'family.txt').write_text('mode=alternate\nprimary_key=main\npartner_key=partner\npartner=partner\n')
            process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, env=env,
                                       creationflags=subprocess.CREATE_NO_WINDOW)
            stdout, stderr = process.communicate(timeout=90)
            assert process.returncode == 0, (command, stdout, stderr)
            if command[0] == probe:
                assert not list(Path(tempfile.gettempdir()).glob(f'FreakMeleeValidation-{process.pid}-*')), 'Validation left temporary asset copies'
                print(stdout.strip())
