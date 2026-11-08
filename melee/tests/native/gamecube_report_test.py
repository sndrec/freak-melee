"""Exercise SDL's actual GameCube USB report parsers without claiming hardware.

Compile the parser functions from the selected SDL source with a report sink.
Identical packets must produce identical axes before and after extreme travel.
An unpatched source can be checked with --expect-drift to reproduce the defect.
"""
import argparse
from pathlib import Path
import subprocess


def function(source, name):
    end = source.index('(', source.index(name))
    start = source.rfind('\nstatic ', 0, end) + 1
    brace = source.index('{', end)
    depth = 1
    finish = brace + 1
    while depth:
        depth += (source[finish] == '{') - (source[finish] == '}')
        finish += 1
    return source[start:finish]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--compiler', default='clang')
    p.add_argument('--expect-drift', action='store_true')
    args = p.parse_args()
    source = args.source.read_text()
    start = source.index('typedef struct')
    end = source.index('} SDL_DriverGameCube_Context;', start) + len('} SDL_DriverGameCube_Context;')
    reset = 'ResetAxisRange' in source
    code = r'''
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t Uint8;
typedef int16_t Sint16;
typedef uint64_t Uint64;
typedef unsigned SDL_JoystickID;
typedef struct { int unused; } SDL_HIDAPI_Device;
typedef struct { Sint16 axes[6]; } SDL_Joystick;
enum { SDL_GAMEPAD_AXIS_LEFTX, SDL_GAMEPAD_AXIS_LEFTY,
       SDL_GAMEPAD_AXIS_RIGHTX, SDL_GAMEPAD_AXIS_RIGHTY,
       SDL_GAMEPAD_AXIS_LEFT_TRIGGER, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,
       SDL_GAMEPAD_AXIS_COUNT };
#define MAX_CONTROLLERS 4
#define SDL_MIN_SINT16 (-32768)
#define SDL_MAX_SINT16 32767
#define SDL_memset memset
static SDL_Joystick sticks[4];
static Uint64 SDL_GetTicksNS(void) { return 1; }
static SDL_Joystick *SDL_GetJoystickFromID(unsigned id) { return id && id<=4 ? &sticks[id-1] : NULL; }
static void SDL_SendJoystickButton(Uint64 t,SDL_Joystick *j,int b,bool down) {}
static void SDL_SendJoystickAxis(Uint64 t,SDL_Joystick *j,int a,Sint16 v) { j->axes[a]=v; }
static void HIDAPI_JoystickConnected(SDL_HIDAPI_Device *d,unsigned *id) { abort(); }
static void HIDAPI_JoystickDisconnected(SDL_HIDAPI_Device *d,unsigned id) { abort(); }
static float HIDAPI_RemapVal(float v,float a,float b,float c,float d) { return c+(v-a)*(d-c)/(b-a); }
'''
    code += source[start:end] + '\n'
    if reset:
        code += function(source, 'ResetAxisRange') + '\n'
    code += function(source, 'HIDAPI_DriverGameCube_HandleJoystickPacket') + '\n'
    code += function(source, 'HIDAPI_DriverGameCube_HandleNintendoPacket') + '\n'
    code += r'''
static void init(SDL_DriverGameCube_Context *c) {
    memset(c,0,sizeof(*c)); memset(sticks,0,sizeof(sticks));
    for(unsigned i=0;i<4;++i) { c->joysticks[i]=i+1; INITIALIZE_RANGE }
}
static void require(bool value,const char *message) {
    if(!value) { fprintf(stderr,"%s\n",message); exit(1); }
}
int main(void) {
    SDL_HIDAPI_Device device={0}; SDL_DriverGameCube_Context ctx;
    Sint16 before[4][6]; unsigned changed=0;
    Uint8 packet[37]={0x21};
    for(unsigned i=0;i<4;++i) {
        Uint8 *s=packet+1+9*i; s[0]=0x10;
        s[3]=130; s[4]=123; s[5]=121; s[6]=108; s[7]=10; s[8]=20;
    }
    init(&ctx);
    HIDAPI_DriverGameCube_HandleNintendoPacket(&device,&ctx,packet,37);
    for(unsigned i=0;i<4;++i) memcpy(before[i],sticks[i].axes,sizeof(before[i]));
    Uint8 extreme[37]; memcpy(extreme,packet,37);
    for(unsigned value=0;value<=255;value+=255) {
        for(unsigned i=0;i<4;++i) memset(extreme+1+9*i+3,value,6);
        HIDAPI_DriverGameCube_HandleNintendoPacket(&device,&ctx,extreme,37);
    }
    HIDAPI_DriverGameCube_HandleNintendoPacket(&device,&ctx,packet,37);
    for(unsigned i=0;i<4;++i) for(unsigned a=0;a<6;++a) changed+=before[i][a]!=sticks[i].axes[a];
    printf("Nintendo rest C-stick Y: %d -> %d\n",before[0][3],sticks[0].axes[3]);
    if(!EXPECT_DRIFT) {
        packet[1+5]++;
        Sint16 previous=sticks[0].axes[2];
        HIDAPI_DriverGameCube_HandleNintendoPacket(&device,&ctx,packet,37);
        require(sticks[0].axes[2]-previous==257,"One ADC step must retain its full resolution");
    }
    for(unsigned invert=0;invert<2;++invert) {
        Uint8 pc[8]={0,0,130,123,108,121,10,20}; init(&ctx);
        HIDAPI_DriverGameCube_HandleJoystickPacket(&device,&ctx,0,pc,invert);
        memcpy(before[0],sticks[0].axes,sizeof(before[0]));
        Uint8 edge[8]={0};
        for(unsigned value=0;value<=255;value+=255) {
            memset(edge+2,value,6);
            HIDAPI_DriverGameCube_HandleJoystickPacket(&device,&ctx,0,edge,invert);
        }
        HIDAPI_DriverGameCube_HandleJoystickPacket(&device,&ctx,0,pc,invert);
        for(unsigned a=0;a<6;++a) changed+=before[0][a]!=sticks[0].axes[a];
    }
    printf("History-dependent axes across four Nintendo slots and both PC formats: %u\n",changed);
    require(EXPECT_DRIFT ? changed>0 : changed==0,"Report history must match the expected range behavior");
    return 0;
}
'''
    code = code.replace('INITIALIZE_RANGE', 'ResetAxisRange(c,i);' if reset else '')
    code = code.replace('EXPECT_DRIFT', '1' if args.expect_drift else '0')
    root = args.output.resolve(); root.mkdir(parents=True, exist_ok=True)
    translation = root/'reports.c'; translation.write_text(code)
    for platform in ('windows', 'other'):
        exe = root/f'reports-{platform}.exe'
        command = [args.compiler, '-std=c11', '-O2', str(translation), '-o', str(exe)]
        if platform=='windows': command.append('-DSDL_PLATFORM_WIN32=1')
        subprocess.run(command, check=True)
        subprocess.run([str(exe)], check=True)


if __name__=='__main__': main()
