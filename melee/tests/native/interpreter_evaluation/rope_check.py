"""Execute actual authored Gumstrings loops against controlled resource storage."""
import argparse
import json
from pathlib import Path
from prepare import ROOT,HERE,run

HARNESS=r'''
#include <stdio.h>
#include <stdlib.h>
static float resource_values[40][NativeArticleResource_FloatCount];
static float resource_matrices[40][12], resource_joints[40][3];
static bool resource_draw[40];
static uint64_t digest=UINT64_C(1469598103934665603);
static unsigned observations;
static void hash_bytes(const void* address,size_t count) {
    const unsigned char* bytes=address;
    for(size_t i=0;i<count;++i){digest^=bytes[i];digest*=UINT64_C(1099511628211);}
}
static void valid(unsigned node){if(node>=40)abort();++observations;}
static float resource_read(HSD_GObj* g,unsigned node,NativeArticleResourceFloat field) {
    (void)g;valid(node);if(field>=NativeArticleResource_FloatCount)abort();
    return resource_values[node][field];
}
static bool resource_write(HSD_GObj* g,unsigned node,NativeArticleResourceFloat field,float value) {
    (void)g;valid(node);if(field>=NativeArticleResource_FloatCount)abort();
    resource_values[node][field]=value;return true;
}
static bool draw(HSD_GObj* g,unsigned node,bool active){(void)g;valid(node);resource_draw[node]=active;return true;}
static bool attachment(HSD_GObj* g,unsigned node,const Vec3* offset,Vec3* point) {
    (void)g;valid(node);*point=(Vec3){resource_values[node][0]+offset->x,
        resource_values[node][1]+offset->y,resource_values[node][2]+offset->z};return true;
}
static void joint(HSD_GObj* g,unsigned node,const char* name,unsigned component,float value) {
    (void)g;valid(node);if(*name||component>=3)abort();resource_joints[node][component]=value;
}
static bool matrix(HSD_GObj* g,unsigned node,const Mtx value) {
    (void)g;valid(node);memcpy(resource_matrices[node],value,sizeof(Mtx));return true;
}
int main(void) {
    Item item={0};HSD_GObj object={0};object.user_data=&item;
    NativeFighterHost host={.article_resource_read_float=resource_read,
        .article_resource_write_float=resource_write,.article_resource_set_draw=draw,
        .article_resource_read_attachment_point=attachment,.article_resource_write_joint=joint,
        .article_resource_set_world_matrix=matrix,.sqrt_float=sqrtf};engine=&host;
    for(unsigned i=0;i<sizeof(script_parameters)/sizeof(script_parameters[0]);++i)script_parameters[i].f=2.25f;
    for(unsigned sample=0;sample<128;++sample) {
        memset(resource_values,0,sizeof(resource_values));memset(resource_matrices,0,sizeof(resource_matrices));
        memset(resource_joints,0,sizeof(resource_joints));memset(resource_draw,0,sizeof(resource_draw));
        memset(item.xDD4_itemVar.native_article.state,0,sizeof(item.xDD4_itemVar.native_article.state));
        int tail=(int)(sample%40);scripted_article_set_state_int(&item,116,tail);
        scripted_article_set_state_float(&item,100,0.25f);scripted_article_set_state_float(&item,104,-0.5f);
        scripted_article_set_state_float(&item,108,0.75f);scripted_article_set_state_float(&item,112,0.125f*(sample%7));
        for(unsigned n=0;n<=(unsigned)tail;++n) {
            resource_values[n][0]=0.25f*n;resource_values[n][1]=1.5f+0.375f*n;
            resource_values[n][2]=0.0625f*(n+sample);resource_values[n][3]=-0.25f;
            resource_values[n][4]=0.125f;resource_values[n][5]=-0.0625f;
            script_state_array_set_bool(item.xDD4_itemVar.native_article.state,0,(int)n,40,(n+sample)%3!=0);
        }
        scripted_article_2_helper_5(&object,(int32_t)(sample%(tail+1)));
        scripted_article_2_helper_6(&object,0,1.375f,(sample&1)!=0);
        bool complete=scripted_article_2_helper_9(&object);
        scripted_article_2_helper_14(&object);
        hash_bytes(resource_values,sizeof(resource_values));hash_bytes(resource_matrices,sizeof(resource_matrices));
        hash_bytes(resource_joints,sizeof(resource_joints));hash_bytes(resource_draw,sizeof(resource_draw));
        hash_bytes(item.xDD4_itemVar.native_article.state,sizeof(item.xDD4_itemVar.native_article.state));
        hash_bytes(&complete,sizeof(complete));
    }
    uint64_t stats[4];FreakMelee_EvalStats(stats);
    printf("{\"digest\":\"%016llx\",\"cases\":128,\"resource_observations\":%u,\"entries\":%llu,\"native_calls\":%llu,\"interpreted_calls\":%llu,\"errors\":%llu}\n",
        digest,observations,stats[0],stats[1],stats[2],stats[3]);return stats[3]?1:0;
}
'''

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepared',type=Path,required=True)
    parser.add_argument('--compiler',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();out=args.output.resolve()
    if out.drive.upper()!='A:' or out.exists():parser.error('Use fresh output on A:')
    out.mkdir(parents=True);commands=[];results={}
    for member in ['popo','nana']:
        modes={}
        for mode in ['native','interpreted']:
            source=out/(member+'-'+mode+'.c');exe=out/(member+'-'+mode+'.exe')
            source.write_text((args.prepared/member/(mode+'.c')).read_text()+HARNESS,newline='\n')
            cmd=[args.compiler,'-std=gnu11','-O2','-ffp-contract=off','-fuse-ld=lld','-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS',
                '-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(HERE),source]
            if mode=='interpreted':cmd+=[HERE/'vm_runtime.c']
            commands.append(run(cmd+['-o',exe],out,member+'-'+mode+'-compile'))
            commands.append(run([exe],out,member+'-'+mode+'-run'))
            modes[mode]=json.loads((out/(member+'-'+mode+'-run.log')).read_text())
        for key in ['digest','resource_observations']:
            if modes['native'][key]!=modes['interpreted'][key]:raise RuntimeError('Actual rope parity mismatch '+member+' '+key)
        if modes['interpreted']['interpreted_calls']==0:raise RuntimeError('No authored nested VM calls executed')
        results[member]=modes
    receipt={'status':'PASS actual authored Gumstrings loops/nested helpers/arrays/aggregate outputs native vs interpreted bytes',
        'results':results,'commands':commands,
        'limits':'Correctness fixture with controlled flat resource storage and native sqrt. No stock physics comparison, game timing, imported resource graphs, lifetime or rollback acceptance.'}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',newline='\n');print(json.dumps(results))

if __name__=='__main__':main()
