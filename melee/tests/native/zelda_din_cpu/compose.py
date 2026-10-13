"""New paired Zelda output, preserving the accepted SDK68 source namespace."""
import importlib.util
from pathlib import Path
import re

HERE = Path(__file__).resolve().parent


def compose():
    path = HERE.parent/'paired_form_composition/compose.py'
    spec = importlib.util.spec_from_file_location('paired_source', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    source = module.compose('zelda')
    fragment = (HERE/'policy.fms').read_text()
    declarations, rest = fragment.split('# DECLARATIONS\n',1)[1].split('# FUNCTIONS\n',1)
    functions, callbacks = rest.split('# CALLBACKS\n',1)
    first = re.search(r'(?m)^func ',source).start()
    source = source[:first]+declarations+'\n'+source[first:]
    first = re.search(r'(?m)^move ',source).start()
    source = source[:first]+functions+'\n'+source[first:]
    first = re.search(r'(?m)^article ',source).start()
    return (source[:first]+callbacks+'\n'+source[first:]).rstrip()+'\n'
