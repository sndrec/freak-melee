"""Extract the native reader/ancestry and unchanged canonical trail bodies."""
import hashlib, importlib.util, json, re
from pathlib import Path
HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('trail_body', HERE.parent/'script_form_feedback/extract.py')
module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)

def extract(root, out):
    hashes = {}
    def read(path):
        p = root/path; hashes[str(p)] = hashlib.sha256(p.read_bytes()).hexdigest()
        return p.read_text()
    props = read('src/melee/ft/native_script_binding_properties.c')
    guards = '\n'.join(module.body(props, n) for n in ['error', 'nativeScriptBindings_SourceAlive', 'nativeScriptBindings_SourceFighterLayout'])
    guards = re.sub(r'\berror\b', 'trail_guard_error', guards)
    boundary = read('tests/native/script_fighter_scheduler/boundary.c')
    boundary = boundary.replace(module.body(boundary, 'nativeScriptBindings_SourceAlive'), '')
    boundary = boundary.replace(module.body(boundary, 'nativeScriptBindings_Read'), '')
    definition = module.body(boundary, 'nativeFighterRuntime_Definition')
    boundary = boundary.replace(definition, definition.replace('{', '{++trail_legacy_queries;', 1))
    boundary = 'extern unsigned trail_legacy_queries;\n'+boundary
    (out/'boundary.c').write_text('#include <melee/ft/native_source_package.h>\n#include <melee/ft/ftdat.h>\n'+boundary+'\n'+guards+'\n')
    native = module.body(read('src/melee/ft/ftafterimage.c'), 'read_trail')
    ancestry = module.body(read('src/sysdolphin/baselib/class.c'), 'hsdObjIsDescendantOf')
    (out/'native_trail.inc').write_text(ancestry+'\n'+native+'\n')
    sources = []
    for name in ['link_full.fms', 'young_link_full.fms']:
        text = read('examples/'+name)
        m = re.search(r'^action SwordRibbon:\n    on trail:\n(?:        [^\n]*\n)+', text, re.M)
        if not m: raise RuntimeError('missing canonical trail '+name)
        sources.append(m.group())
    (out/'trail_sources.h').write_text('\n'.join('static const char* trail_source_'+str(i)+' = '+json.dumps(s)+';' for i,s in enumerate(sources))+'\n')
    return hashes
