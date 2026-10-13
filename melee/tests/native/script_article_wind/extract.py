"""Complete common wind pool bodies, with one explicit post-kernel test probe."""
from pathlib import Path
import importlib.util

def extract(root: Path, out: Path):
    out.mkdir(parents=True, exist_ok=True)
    path = root / 'tests/native/script_form_feedback/extract.py'
    spec = importlib.util.spec_from_file_location('feedback_extract', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    # Actual descriptor and Fighter source guard excerpts; no successful fake
    # SourceFighterLayout path in the real Item fixture.
    properties = (root / 'src/melee/ft/native_script_binding_properties.c').read_text()
    guards = [module.body(properties, name) for name in
              ['error', 'nativeScriptBindings_SourceFighterLayout']]
    import re
    guards = [re.sub(r'\berror\b', 'wind_guard_error', text) for text in guards]
    (out / 'fighter_guard.inc').write_text('\n'.join(guards) + '\n', encoding='utf-8')
    native = (root / 'src/melee/lb/lb_00F9.c').read_text()
    kernels = '\n\n'.join(module.body(native, name) for name in ['inlineA0', 'inlineA1', 'lb_800100B0'])
    wind = module.body(native, 'lb_800119DC')
    reference = wind.replace('lb_800119DC(', 'wind_reference(', 1)
    instrumented = wind[:-1] + '\n    fixture_after_wind();\n}'
    (out / 'wind_native.inc').write_text(kernels + '\n\n' + reference + '\n\n' + instrumented + '\n', encoding='utf-8')
    # The independent legacy Fighter fixture cannot admit Item lifetimes.
    (out / 'absent_item.c').write_text('''#include <melee/it/native_script_item_actor.h>
bool nativeScriptItem_IsAttached(Item_GObj* o){(void)o;return false;}
NativeScriptVMStatus nativeScriptItem_Status(Item_GObj* o){(void)o;return NativeScriptVM_SourceRetired;}
const NativeScriptItemProgram* nativeScriptItem_Spec(Item_GObj* o){(void)o;return NULL;}
bool nativeScriptItem_View(Item_GObj* o,uint32_t g,NativeScriptReceiverView* v){(void)o;(void)g;(void)v;return false;}
''', encoding='utf-8')
    return module
