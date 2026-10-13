"""Compare actual Popo/Nana authored scalar functions before game evaluation.

This is a correctness fixture, never an interpreter speed benchmark.
"""
import argparse
import json
from pathlib import Path
import re
from prepare import ROOT,HERE,run

HARNESS=r'''
#include <stdio.h>
static uint64_t digest=UINT64_C(1469598103934665603);
static void hash_bytes(const void* address,size_t count) {
    const unsigned char* bytes=address;
    for(size_t i=0;i<count;++i){digest^=bytes[i];digest*=UINT64_C(1099511628211);}
}
int main(void) {
    Fighter fighter={0};HSD_GObj object={0};object.user_data=&fighter;
    script_state_base=0;script_parameters[THRESHOLD].f=0.25f;
    for(unsigned sample=0;sample<4096;++sample) {
        float speed=((int)(sample%97)-48)*0.0625f;
        float accel=((int)(sample%23)-11)*0.09375f;
        float target=sample%7==0?0:((int)(sample%41)-20)*0.125f;
        fighter.self_vel.x=speed;fighter.gr_vel=-speed;
        fighter.input.lstick[0].x=((int)(sample%33)-16)*0.03125f;
        script_fn_squall_input(&object,0.1875f);
        script_fn_squall_accel(&object,accel,target);
        script_fn_linked_calc_ground_accel(&object,accel,target);
        hash_bytes(&fighter.x74_self_accel.x,4);
        hash_bytes(&fighter.xE4_ground_accel_1,4);
        float input=script_state_get_float(&fighter,script_state_base+28);
        hash_bytes(&input,4);
        fighter.x2219_b5=(sample&1)!=0;
        bool flag=script_fn_linked_hitlag_active(&object);hash_bytes(&flag,sizeof(flag));
        fighter.cur_pos=(Vec3){speed,accel,target};
        float x=script_fn_linked_position_x(&object);
        float y=script_fn_linked_position_y(&object);
        float z=script_fn_linked_position_z(&object);
        hash_bytes(&x,4);hash_bytes(&y,4);hash_bytes(&z,4);
    }
    uint64_t stats[4];FreakMelee_EvalStats(stats);
    printf("{\"digest\":\"%016llx\",\"cases\":4096,\"entries\":%llu,\"native_calls\":%llu,\"interpreted_calls\":%llu,\"errors\":%llu}\n",
        digest,stats[0],stats[1],stats[2],stats[3]);
    return stats[3]?1:0;
}
'''

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepared',type=Path,required=True)
    parser.add_argument('--compiler',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();out=args.output.resolve()
    if out.drive.upper()!='A:' or out.exists():parser.error('Use fresh output on A:')
    out.mkdir(parents=True)
    commands=[];results={}
    for member in ['popo','nana']:
        original=(args.prepared/member/'source.c').read_text()
        threshold=re.search(r'fabsf\(fighter->input.lstick\[0\].x\) >= script_parameters\[(\d+)\]',original)
        if not threshold:raise RuntimeError('Actual Squall threshold not found')
        modes={}
        for mode in ['native','interpreted']:
            source=out/(member+'-'+mode+'.c')
            source.write_text((args.prepared/member/(mode+'.c')).read_text()+HARNESS.replace('THRESHOLD',threshold[1]),newline='\n')
            exe=out/(member+'-'+mode+'.exe')
            cmd=[args.compiler,'-std=gnu11','-O2','-ffp-contract=off','-fuse-ld=lld','-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS',
                '-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(HERE),source]
            if mode=='interpreted':cmd+=[HERE/'vm_runtime.c']
            cmd+=['-o',exe]
            commands.append(run(cmd,out,member+'-'+mode+'-compile'))
            commands.append(run([exe],out,member+'-'+mode+'-run'))
            modes[mode]=json.loads((out/(member+'-'+mode+'-run.log')).read_text())
        if modes['native']['digest']!=modes['interpreted']['digest']:raise RuntimeError('Actual scalar parity mismatch '+member)
        results[member]=modes
    receipt={'status':'PASS actual Squall scalar/position/bitfield decisions, native vs interpreted byte results',
        'results':results,'commands':commands,
        'limits':'Scalar correctness only; Host physics, recipient dispatch and complete game rollback are not exercised by this fixture.'}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',newline='\n')
    print(json.dumps(results))

if __name__=='__main__':main()
