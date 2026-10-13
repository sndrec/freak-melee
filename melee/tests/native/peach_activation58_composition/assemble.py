"""Compose the accepted full Peach source as an unregistered independent candidate."""
from pathlib import Path
import re

HERE = Path(__file__).resolve().parent
COMPONENT = HERE.parent / 'peach_parasol_travel_composition/candidate.fms'


def compose():
    source = COMPONENT.read_text(encoding='utf-8')
    assert source.count('base ftDataPeach 341 30') == 1
    source = re.sub(r'^# UNREGISTERED inherited.*\n# Composed.*\n', '', source)
    # The original Item GX table already selects the shared renderer for each
    # of these kinds. Declare this explicitly; no private item-kind draw policy.
    source, count = re.subn(r'(?m)^(article It_Kind_Peach_\w+ states \d+:)$',
                            r'\1\n    render standard', source)
    assert count == 5
    return ('# UNREGISTERED independent versus candidate; no installed/default change.\n'
            '# Debug-ROM cosmetics and normal common taunt are authored.\n'
            'fighter:\n    behavior independent\n\n' + source + '\n' + (HERE / 'taunt.fms').read_text(encoding='utf-8'))
