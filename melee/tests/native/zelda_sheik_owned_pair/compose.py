"""Independent proposed source outputs; accepted component files are read-only."""
import importlib.util
from pathlib import Path

HERE = Path(__file__).resolve().parent


def compose(member):
    path = HERE.parent/('zelda_din_cpu/compose.py' if member=='zelda' else 'paired_form_composition/compose.py')
    spec = importlib.util.spec_from_file_location('accepted_'+member,path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    source = module.compose() if member=='zelda' else module.compose(member)
    prefix = '# PROPOSED owned pair member; unregistered, no gameplay activation.\n'
    source = prefix+'fighter:\n    behavior independent\n\n'+source
    return source.rstrip()+'\n'
