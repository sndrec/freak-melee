"""Exact allocator bodies; omit unrelated animation/math/logging methods."""
import re

def function(text, name):
    match = re.search(r'(?m)^[^\n;{}]*\b'+re.escape(name)+r'\([^;{}]*\)\s*\{',text)
    if not match: raise RuntimeError('Missing body '+name)
    opening = text.index('{',match.start()); depth = 1; at = opening+1
    state = None
    while depth:
        if at >= len(text): raise RuntimeError('Unbalanced body '+name)
        c = text[at]; n = text[at:at+2]
        if state in ('"',"'"):
            if c == '\\': at += 2; continue
            if c == state: state = None
        elif state == '//':
            if c == '\n': state = None
        elif state == '/*':
            if n == '*/': state = None; at += 2; continue
        elif n in ('//','/*'): state = n; at += 2; continue
        elif c in ('"',"'"): state = c
        elif c == '{': depth += 1
        elif c == '}': depth -= 1
        at += 1
    return text[match.start():at]

def extract(root, out):
    baselib = root/'src/sysdolphin/baselib'
    mtx = (baselib/'mtx.c').read_text()
    robj = (baselib/'robj.c').read_text()
    names = ['HSD_VecAlloc','HSD_VecFree','HSD_MtxAlloc','HSD_MtxFree',
             'HSD_VecGetAllocData','HSD_VecInitAllocData',
             'HSD_MtxGetAllocData','HSD_MtxInitAllocData']
    native = '#include <sysdolphin/baselib/mtx.h>\n#include <sysdolphin/baselib/robj.h>\n'
    native += 'HSD_ObjAllocData HSD_Mtx_804C2310, HSD_Mtx_804C233C;\n'
    native += 'HSD_ObjAllocData robj_alloc_data, rvalue_alloc_data;\n'
    native += '\n'.join(function(mtx,n) for n in names)
    native += '\n'+'\n'.join(function(robj,n) for n in
                 ['HSD_RObjGetAllocData','HSD_RvalueObjGetAllocData','HSD_RObjInitAllocData'])
    (out/'satellites.c').write_text(native+'\n')
    os = root.parent/'aurora/lib/dolphin/os/OSAlloc.cpp'
    text = os.read_text()
    text = text.replace(function(text,'OSDumpHeap'),'')
    text = text.replace('#include "../../logging.hpp"','')
    text = text.replace('static aurora::Module AllocLog("aurora::os::alloc");','')
    (out/'OSAlloc.cpp').write_text(text)
    texp = (baselib/'texp.c').read_text()
    for name in ['HSD_TExpSetReg','HSD_TExpSetupTev']:
        texp = texp.replace(function(texp,name),'')
    texp = texp.replace('static GXTevKColorID id1[4] = { GX_KCOLOR0, GX_KCOLOR1, GX_KCOLOR2,\n                                GX_KCOLOR3 };','')
    texp = texp.replace('static GXTevRegID id2[3] = { GX_TEVREG0, GX_TEVREG1, GX_TEVREG2 };','')
    (out/'texp.c').write_text(texp)
    material = '#include <sysdolphin/baselib/mobj.h>\n#include <sysdolphin/baselib/tev.h>\n#include <sysdolphin/baselib/class.h>\n'
    tobj = (baselib/'tobj.c').read_text()
    for name in ['HSD_TexMapID2PTTexMtx','MakeColorGenTExp','TObjMakeTExp',
                 'HSD_TObjAssignResources','HSD_Index2TexCoord','HSD_Index2TexMap']:
        material += function(tobj,name)+'\n'
    material += 'HSD_TObjInfo hsdTObj = {.make_texp=TObjMakeTExp};\n'
    mobj = (baselib/'mobj.c').read_text()
    material += function(mobj,'MObjMakeTExp')+'\n'
    material += 'HSD_MObjInfo hsdMObj = {.make_texp=MObjMakeTExp};\n'
    material += 'HSD_TObj *tobj_toon, *tobj_shadows;\n'
    material += function(mobj,'HSD_MObjCompileTev')+'\n'
    (out/'material.c').write_text(material)
    (out/'stage.c').write_text('#include <sysdolphin/baselib/tev.h>\n'+
                              function((baselib/'tev.c').read_text(),'HSD_Index2TevStage')+'\n')
    return [baselib/(n+'.c') for n in ['mtx','robj','texp','tobj','mobj','tev']]+[os]
