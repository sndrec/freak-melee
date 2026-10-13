"""Actual sound resolver/channel bodies for isolated presentation testing."""
import hashlib,importlib.util
from pathlib import Path
HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('presentation_extract',HERE.parent/'script_kinetics/extract_primitives.py');reference=importlib.util.module_from_spec(spec);spec.loader.exec_module(reference)
FUNCTIONS={
 'src/melee/ft/ft_0877.c':['ft_80087C70','ft_80087D0C','ft_PlaySFX'],
 'src/melee/ft/ft_0881.c':['ft_800881D8','ft_80088328','ft_80088478','ft_80088510','ft_800885A8','ft_80088640','ft_80088770','ft_800887CC','ft_80088828','ft_80088884','ft_800888E0','ft_8008893C'],
 'src/melee/ft/native_fighter_runtime.c':['nativeFighterRuntime_SoundMuteChannels'],
 'src/sysdolphin/baselib/jobj.c':['HSD_JObjSetMtxDirtySub','HSD_JObjGetFlags','HSD_JObjClearFlags'],
 'src/sysdolphin/baselib/class.c':['hsdObjIsDescendantOf'],
 'extern/dolphin/src/dolphin/mtx/mtxvec.c':['C_MTXMultVec'],
}
def extract(root,output):
 pieces=[];inputs={}
 for relative,names in FUNCTIONS.items():
  path=root/relative;source=path.read_text();inputs[str(path.resolve())]=hashlib.sha256(path.read_bytes()).hexdigest()
  for name in names:
   body=reference.body(source,name)
   # The checkout declaration is Mtx (3x4); this original source spells the
   # argument Mtx44 (4x4). Both decay to float (*)[4]; only the declaration's
   # row bound changes, never the actual arithmetic/element access.
   if name=='C_MTXMultVec':body=body.replace('Mtx44 m','Mtx m',1)
   pieces.append(body)
 output.write_text('// Actual checkout bodies, no native-authored script replacements.\n#pragma clang diagnostic push\n#pragma clang diagnostic ignored "-Wunused-variable"\n'+ '\n\n'.join(pieces)+'\n#pragma clang diagnostic pop\n',newline='\n')
 return inputs
