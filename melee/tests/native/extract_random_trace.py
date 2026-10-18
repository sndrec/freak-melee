"""Extract the production RNG diagnostics without the fixture's controlled RNG."""
import argparse
import hashlib
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source', required=True, type=Path)
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
source = args.source.read_text()
start = source.index('#pragma clang section')
end = source.index('#endif', start)
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text('#include <sysdolphin/baselib/random.h>\n'
                       '#include <stdint.h>\n#include <stdbool.h>\n'
                       '#define WIN32_LEAN_AND_MEAN\n#include <windows.h>\n'
                       '#include <stdlib.h>\n#include <stdio.h>\n#include <string.h>\n'
                       + source[start:end], encoding='utf-8')
print(json.dumps(dict(source_sha256=hashlib.sha256(args.source.read_bytes()).hexdigest())))
