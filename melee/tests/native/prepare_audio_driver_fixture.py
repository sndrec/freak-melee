"""Compile the production AX start/stop functions without a device or disc loader."""
import sys
from pathlib import Path


source = Path(sys.argv[1]).read_text()


def offset(marker):
    if source.count(marker) != 1:
        raise ValueError('AX driver fixture boundary changed: ' + marker)
    return source.index(marker)


start = offset('void AXDriverUnlink(')
stops_end = offset('#ifdef MUST_MATCH')
starts = offset('static inline HSD_SM* AXDriver_8038CFF4_inline(')
starts_end = offset('bool AXDriver_8038D4E4(')
definitions = ''.join(line + '\n' for line in source.splitlines()
                      if line.startswith(('#define MIN2(', '#define MAX2(', '#define CLAMP(')))
output = Path(sys.argv[2])
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(source[:start] + definitions + source[start:stops_end] + source[starts:starts_end])
