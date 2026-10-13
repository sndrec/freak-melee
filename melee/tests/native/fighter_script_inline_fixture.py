"""Compile a test fixture with inline tuning defaults, preserving DAT references."""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('reference_exporter')
parser.add_argument('source', type=Path)
parser.add_argument('key')
parser.add_argument('output', type=Path)
parser.add_argument('--construction', nargs=3, metavar=('ROOT','FIRST','COUNT'))
args = parser.parse_args()
source = args.source.read_text()
values = re.findall(r'^parameter ((?:float|int|bool) \w+ = [^\n]+)$', source, re.MULTILINE)
source = re.sub(r'^parameter (?:float|int|bool) \w+ = [^\n]+\n?', '', source, flags=re.MULTILINE)
with tempfile.TemporaryDirectory(prefix='freak-script-fixture-') as directory:
    root = Path(directory)
    (root / 'behavior.fms').write_text(source)
    (root / 'fighter_parameters.fmp').write_text('\n'.join(values) + '\n')
    command=[args.reference_exporter, str(root / 'behavior.fms'), args.key,
             str(args.output.resolve()), str(root), str(root / 'fighter_parameters.fmp')]
    if args.construction:command += ['--construction', *args.construction]
    subprocess.run(command, check=True, creationflags=subprocess.CREATE_NO_WINDOW)
