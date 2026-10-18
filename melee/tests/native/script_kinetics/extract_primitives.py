"""Extract actual generic engine bodies for isolated kinetic binding fixtures."""
import argparse
import hashlib
import json
from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[3]
FUNCTIONS={
    'src/melee/ft/native_script_moves.c':['nativeScriptMoves_Contains'],
    'src/melee/ft/ftmovement.c':[
        'ftCommon_CalcGroundAccel_Deaccel','ftCommon_CalcGroundAccel_AccelToVel',
        'ftCommon_CalcGroundAccel_AccelToLStickX','ftCommon_ClampGroundVel',
        'ftCommon_CalcSelfAccel_Deaccel','ftCommon_CalcSelfAccel_DeaccelAir',
        'ftCommon_CalcSelfAccel_DeaccelQuickAir','ftCommon_CalcSelfAccel_DeaccelQuick',
        'ftCommon_CalcSelfAccel_AccelToVelClamped','ftCommon_CalcSelfAccel_AccelToVelClampedFrom',
        'ftCommon_CalcSelfAccel_Drift','ftCommon_CalcSelfAccel_DriftFrom',
        'ftCommon_CalcSelfAccel_AccelToVel','ftCommon_CalcSelfAccel_DriftSimple',
        'ftCommon_CalcSelfAccel_DriftSimple_NoFriction','ftCommon_ClampSelfVelX',
        'ftCommon_ClampAirDrift','ftCommon_Fall','ftCommon_FallBasic','ftCommon_FallFast',
        'ftCommon_ClampFallSpeed','ftCommon_Ascend',
        'ftCommon_SetSelfMovementFromGroundedMovement','ft_GetGroundFrictionMultiplier'],
    'src/melee/ft/ftcommon.c':['ftCommon_CheckFallFast','_func_8007E2FC_inline','ftCommon_8007E2FC'],
    'src/melee/ft/ft_084E.c':['ft_80084EEC','ft_80084F3C','ft_80084FA8',
        'ft_80085030','ft_80085088','ft_800850E0','ft_80085134','ft_80085154','ft_800851C0'],
    'src/melee/ft/ft_081B.c':['ft_80084DB0'],
    'src/melee/ft/ftanim.c':['ftAnim_IsFramesRemaining','ftAnim_8006F484',
        'ftAnim_8006F0FC_inline','ftAnim_8006F0FC','ftAnim_SetAnimRate','ftAnim_8006EBA4'],
    'src/melee/lb/lbcollision.c':['lbColl_80008428'],
    'src/melee/ft/native_hitbox_lifetime.c':['nativeHitboxLifetime_Clear',
        'nativeHitboxLifetime_BeginStep','nativeHitboxLifetime_EndStep'],
    'src/melee/ft/ftcoll.c':['ftColl_8007AFC8'],
}

def native_only(source):
    """Select known MELEE_NATIVE branches, preserving unrelated directives.

    This is branch selection rather than C preprocessing: source bodies and
    unknown macro expressions remain unchanged, so fixture leaves stay explicit.
    """
    output=[];stack=[];active=True
    for line in source.splitlines(keepends=True):
        directive=re.match(r'\s*#\s*(ifdef|ifndef|if|elif|else|endif)\b(.*)',line)
        if directive:
            kind,condition=directive.groups();condition=condition.strip()
            if kind in ('ifdef','ifndef','if'):
                known=condition in ('MELEE_NATIVE','defined(MELEE_NATIVE)','defined MELEE_NATIVE','!defined(MELEE_NATIVE)','!defined MELEE_NATIVE')
                selected=kind!='ifndef' and not condition.startswith('!')
                stack.append((active,known,selected))
                if known:active=active and selected;continue
            elif kind=='else' and stack:
                parent,known,selected=stack[-1]
                if known:active=parent and not selected;continue
            elif kind=='elif' and stack and stack[-1][1]:
                raise ValueError('Native fixture branch needs explicit #elif evaluation: '+condition)
            elif kind=='endif' and stack:
                parent,known,_=stack.pop()
                if known:active=parent;continue
        if active:output.append(line)
    if stack:raise ValueError('Unterminated source preprocessor conditional')
    return ''.join(output)

def body(source,name):
    source=native_only(source)
    match=re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*?\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{',source)
    if not match:raise ValueError('Missing actual engine function '+name)
    cursor=match.end();depth=1
    while depth:
        if cursor>=len(source):raise ValueError('Unbalanced actual engine function after native branch selection: '+name)
        if source.startswith('//',cursor):
            newline=source.find('\n',cursor+2);cursor=len(source) if newline<0 else newline;continue
        if source.startswith('/*',cursor):cursor=source.index('*/',cursor+2)+2;continue
        if source[cursor] in ('"',"'"):
            quote=source[cursor];cursor+=1
            while cursor<len(source) and source[cursor]!=quote:cursor+=2 if source[cursor]=='\\' else 1
            if cursor>=len(source):raise ValueError('Unterminated string in actual engine function '+name)
            cursor+=1;continue
        depth+=(source[cursor]=='{')-(source[cursor]=='}');cursor+=1
    return source[match.start():cursor]

def extract(root,output):
    pieces=[];inputs={};functions=[]
    for relative,names in FUNCTIONS.items():
        path=root/relative;source=path.read_text();inputs[str(path.resolve())]=hashlib.sha256(path.read_bytes()).hexdigest()
        for name in names:
            original=body(source,name)
            if name=='nativeScriptMoves_Contains':
                original='#ifndef SCRIPT_FIXTURE_FULL_MOVE_UNIT\n'+original+'\n#endif'
            pieces.append(original);functions.append(name)
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text('// Actual engine bodies; isolated fixture only.\n'+
        '#include <melee/ft/native_fighter_lifetime.h>\n'+
        '#include <melee/ft/native_hitbox_lifetime.h>\n#include <melee/ft/ftcoll.h>\n#include <melee/lb/lbcollision.h>\n'+
        '#pragma clang diagnostic push\n#pragma clang diagnostic ignored "-Wunused-variable"\n'+
        '#pragma clang diagnostic ignored "-Wunused-parameter"\n#pragma clang diagnostic ignored "-Wsign-compare"\n'+
        '\n\n'.join(pieces)+'\n#pragma clang diagnostic pop\n',newline='\n')
    return {'inputs':inputs,'functions':functions,'extracted_sha256':hashlib.sha256(output.read_bytes()).hexdigest()}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root',type=Path,default=ROOT)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();receipt=extract(args.source_root.resolve(),args.output.resolve())
    print(json.dumps({'functions':len(receipt['functions']),'sha256':receipt['extracted_sha256']}))

if __name__=='__main__':main()
