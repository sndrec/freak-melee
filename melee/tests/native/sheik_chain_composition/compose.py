"""Assemble an unregistered source-only Chain/Needles candidate; no activation."""
from pathlib import Path
import argparse

HERE = Path(__file__).resolve().parent


def compose():
    needles = (HERE.parent / 'sheik_script_composition/needles.fms').read_text()
    chain = (HERE / 'fighter.fms').read_text()
    needle_move = needles.index('move Needles:')
    chain_move = chain.index('move Chain:')
    needle_functions = needles.index('func ')
    chain_functions = chain.index('func ')
    # Only the assembled candidate changes bindings and their pointer queries.
    # Accepted component bytes remain untouched. Actual ftSk_Init cleanup order
    # is implemented once in sheik_resource_cleanup, not as a stock delegate.
    global_needle = needles[:needle_move].replace(
        'callbacks.damage(needle_cleanup)',
        'callbacks.damage(sheik_resource_cleanup)').replace(
        'callbacks.death2(needle_cleanup)',
        'callbacks.death2(sheik_resource_cleanup)').replace(
        'callbacks.damage_is(needle_cleanup)',
        'callbacks.damage_is(sheik_resource_cleanup)').replace(
        'callbacks.death2_is(needle_cleanup)',
        'callbacks.death2_is(sheik_resource_cleanup)')
    return (global_needle[:needle_functions] + chain[:chain_functions] + '\n' +
            global_needle[needle_functions:] + chain[chain_functions:chain_move] + '\n' +
            needles[needle_move:] + '\n' + chain[chain_move:] + '\n' +
            (HERE / 'article.fms').read_text()).rstrip() + '\n'


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(compose(), newline='\n')
