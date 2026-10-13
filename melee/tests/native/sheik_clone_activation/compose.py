"""Combine accepted read-only components; Transform remains a separate gap."""
import argparse
import importlib.util
from pathlib import Path

HERE = Path(__file__).resolve().parent


def split_component(source):
    first_function, first_move, first_article = source.index('func '), source.index('move '), source.index('article ')
    return source[:first_function], source[first_function:first_move], source[first_move:first_article], source[first_article:]


def compose():
    spec = importlib.util.spec_from_file_location('accepted_chain_composition', HERE.parent / 'sheik_chain_composition/compose.py')
    chain_module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(chain_module)
    chain = split_component(chain_module.compose())
    vanish = split_component((HERE.parent / 'sheik_script_composition/vanish.fms').read_text())
    globals_ = chain[0] + vanish[0].replace('base ftDataSeak 341 24\n', '')
    return (globals_ + chain[1] + vanish[1] + chain[2] + vanish[2]
            + (HERE / 'lifecycle.fms').read_text() + '\n' + chain[3] + vanish[3]).rstrip() + '\n'


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(compose(), encoding='utf-8', newline='\n')
