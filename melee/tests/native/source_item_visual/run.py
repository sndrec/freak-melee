"""Actual PNG/WIC resource preparation and billboard draw, no game or renderer."""
import argparse, hashlib, json, struct, subprocess, zlib
from pathlib import Path
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]

def png(seed=0):
    def chunk(name, data):
        return struct.pack('>I',len(data))+name+data+struct.pack('>I',zlib.crc32(name+data)&0xffffffff)
    rgba = b''.join(b'\0'+bytes(v for x in range(5) for v in
        ((17+x*31+seed*7)&255,(9+y*41+seed*3)&255,(x*13+y*19+seed*11)&255,0 if x==0 else 127 if x==1 else 255)) for y in range(3))
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',5,3,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(rgba))+chunk(b'IEND',b'')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();out=args.output.resolve()
    if out.drive.upper()!='A:' or out.exists():parser.error('Use fresh output on A:')
    out.mkdir(parents=True)
    declarations=[]
    invalid_png=bytearray(png(1));invalid_png[-1]^=1
    for name,data in [('png_first',png()),('png_second',png(1)),('png_invalid',invalid_png)]:
        declarations.append('static const uint8_t '+name+'[]={'+','.join(map(str,data))+'};')
    (out/'pixels.h').write_text('\n'.join(declarations)+'\n',newline='\n')
    rebind=ROOT/'src/melee/ft/native_script_item_program.c';source=rebind.read_text()
    start=source.index('void nativeScriptItemPrograms_Rebind(');opening=source.index('{',start);depth=1;end=opening+1
    while depth:
        depth += (source[end]=='{')-(source[end]=='}');end+=1
    (out/'item_rebind_actual.h').write_text(source[start:end]+'\n',newline='\n')
    units=[ROOT/'src/melee/it/native_source_item_visual.c',ROOT/'src/melee/it/native_holdable_visual.c',
        ROOT/'src/melee/ft/native_source_resources.c',HERE/'boundary.c',HERE/'test.c']
    decoder=ROOT/'src/melee/ft/native_gx_image.cpp'
    watched=units+[decoder,rebind,ROOT/'src/melee/it/native_source_item_visual.h',
        ROOT/'src/melee/it/native_script_item_actor.h',ROOT/'src/melee/ft/native_source_resources.h',HERE/'boundary.h']
    def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
    inputs={str(path):sha(path) for path in watched};commands=[]
    shared=['-O2','-ffp-contract=off','-Wall','-Wextra','-Werror','-Wno-array-parameter',
        '-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS',
        '-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out)]
    object_file=out/'decoder.obj';exe=out/'test.exe'
    stages=[('decoder',[args.compiler,'-std=c++17','-fno-exceptions','-fno-rtti']+shared+['-c',decoder,'-o',object_file]),
        ('compile',[args.compiler,'-std=c11','-fuse-ld=lld']+shared+units+[object_file,'-lole32','-lwindowscodecs','-luuid','-o',exe]),
        ('run',[exe])]
    for stage,command in stages:
        command=list(map(str,command));result=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,
            timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
        (out/(stage+'.log')).write_text(result.stdout+result.stderr,newline='\n')
        commands.append({'stage':stage,'command':command,'exit':result.returncode})
        if result.returncode:raise RuntimeError(stage+' failed:\n'+result.stdout+result.stderr)
    for path,value in inputs.items():
        if sha(Path(path))!=value:raise RuntimeError('Changed input '+path)
    receipt={'status':'PASS','result':json.loads(result.stdout),'inputs':inputs,'commands':commands,
        'scope':'Real PNG codec, WIC decoder, tiled pixels, resource lifecycle and actual draw/rebind bodies. Source program/spec/actor-reference registry and GX/camera/model leaves controlled. No actual actor construction, FMS compilation, renderer, game, or full package activation.'}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',newline='\n')
    print(json.dumps(receipt['result']))
if __name__=='__main__':main()
