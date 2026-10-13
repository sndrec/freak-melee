import hashlib,importlib.util,re
from pathlib import Path
def extract(root,out):
 hashes={}
 def src(rel):
  p=root/rel;hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest();return p.read_text()
 path=root/'tests/native/script_form_feedback/extract.py';spec=importlib.util.spec_from_file_location('body',path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 text=src('src/melee/ft/native_script_binding_properties.c');(out/'source_guards.inc').write_text('\n'.join(re.sub(r'\berror\b','lifecycle_guard_error',m.body(text,n)) for n in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout']))
 text=src('src/melee/ft/ftcommon.c');a=text.index('static void (*parasol_table_1');b=text.index('void ftCommon_8007E83C',a);parts=[text[a:b],m.body(text,'ftGetParasolStatus'),m.body(text,'ftCommon_8007E83C')]
 owner=src('src/melee/it/native_item_owner.c');parts += [m.body(owner,n) for n in ['nativeItem_CaptureCreator','nativeItem_Creator']]
 text=src('src/melee/ft/fighter.c');a=text.index('    if (!(flags & Ft_MF_SkipParasol)) {');b=text.index('\n    ftAnim_80070F28',a);parts.append('void held_motion_block(Fighter_GObj* gobj){Fighter* fp=gobj->user_data;MotionFlags flags=0;\n'+text[a:b]+'\n ++native_tail;fp->x213C=99; }')
 (out/'native_held.inc').write_text('\n'.join(parts)+'\n');return hashes
