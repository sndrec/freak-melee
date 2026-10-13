"""Exact current native policy bodies; controlled geometry/response leaves."""
import hashlib,importlib.util
from pathlib import Path
def extract(root,out):
 root=Path(root);out=Path(out);hashes={}
 path=root/'tests/native/script_form_feedback/extract.py';spec=importlib.util.spec_from_file_location('motion_body',path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
 hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 def source(relative):
  path=root/relative;hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest();return path.read_text()
 (out/'properties.c').write_text(source('src/melee/ft/native_script_binding_properties.c'))
 text=source('src/melee/ft/ft_081B.c')
 inline=m.body(text,'ft_80084280_inline');main=m.body(text,'ft_80084280')
 reference=(inline+'\n'+main).replace('ft_80084280_inline','reference_nudge_inline').replace('ft_80084280(','reference_nudge(').replace('MELEE_NATIVE','SCRIPT_REFERENCE_CONSOLE_BRANCH')
 air=m.body(source('src/melee/ft/ftcommon.c'),'ftCommon_8007D5D4')
 collision=m.body(text,'ft_80081D0C')+'\n'+m.body(text,'ftCo_AirCatchHit_Coll')
 reference_collision=collision.replace('MELEE_NATIVE','SCRIPT_REFERENCE_CONSOLE_BRANCH').replace('ft_80081D0C(','reference_air_contact(').replace('ftCo_AirCatchHit_Coll(','reference_air_catch_hit(')
 landing_text=source('src/melee/ft/kinds/ftCommon/ftCo_Landing.c')
 # Duplicate mutually exclusive switch openings confuse the brace extractor.
 # Remove only the inactive console opening; selected native text is unchanged.
 landing_text=landing_text.replace('#else\n    switch (fp->kind) {\n','#else\n    switch (fp->kind)\n')
 landing=m.body(landing_text,'ftCo_Landing_Enter')+'\n'+m.body(landing_text,'ftCo_Landing_Enter_Basic')
 hammer=m.body(source('src/melee/ft/kinds/ftCommon/ftCo_HammerLanding.c'),'ftCo_HammerLanding_Enter')
 hammerwait=m.body(source('src/melee/ft/kinds/ftCommon/ftCo_HammerWait.c'),'inlineA0').replace('inlineA0','hammer_wait_inline')+'\n'+m.body(source('src/melee/ft/kinds/ftCommon/ftCo_HammerWait.c'),'ftCo_800C4ED8').replace('inlineA0','hammer_wait_inline')
 common=source('src/melee/ft/ftcommon.c')
 grounded=m.body(common,'ftCommon_8007D6A4')+'\n'+m.body(common,'ftCommon_8007D7FC')
 state=m.body(source('src/melee/ft/native_script_binding_state.c'),'nativeScriptBindingState_Read').replace('error(', 'state_read_error(')
 (out/'state_read.c').write_text('#include "native_script_bindings.h"\n#include "native_fighter_lifetime.h"\n#include "ft_081B.h"\n#include <sysdolphin/baselib/controller.h>\n#include <melee/mp/forward.h>\n#include <stdio.h>\nstatic bool state_read_error(NativeScriptDiagnostic* d,const char* t){if(d)snprintf(d->message,sizeof(d->message),"%s",t);return false;}\n'+state)
 scaling=source('src/melee/ft/ftchangeparam.c')
 threshold=m.body(scaling,'ftCo_CalcYScaledKnockback')+'\n'+m.body(scaling,'ftCo_800D0EC8')
 native=m.body(source('src/melee/mp/mpcoll.c'),'mpCollSetFacingDir')+'\n'+air+'\n'+air.replace('ftCommon_8007D5D4','reference_ground_to_air')+'\n'+inline+'\n'+main+'\n'+reference+'\n'+collision+'\n'+reference_collision+'\n'+grounded+'\n'+landing+'\n'+hammer+'\n'+hammerwait+'\n'+threshold
 (out/'native_bodies.inc').write_text(native+'\n')
 return hashes
