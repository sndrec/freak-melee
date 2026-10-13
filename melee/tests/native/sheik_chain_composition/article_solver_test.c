/* Four solver matrix; shared controlled resource observations are in the header. */
#include "article_harness.h"

int main(void)
{
    NativeFighterHost host={0};host.callback_source_alive=nativeCallbackSource_Alive;host.sqrt_float=sqrtf;host.abs_float=fabsf;host.article_resource_read_float=resource_read;host.article_resource_write_float=resource_write;host.article_resource_read_int=resource_int;host.article_resource_sweep_point=resource_sweep;host.article_resource_set_draw=resource_draw;host.play_global_sound=lbAudioAx_800237A8;host.item_creator=retained_creator;engine=&host;
    CHECK(FreakMelee_SetFighterParameters(parameter_bits,sizeof(parameter_bits)/sizeof(*parameter_bits),parameter_schema));
#include "article_attributes.inc"
    /* Keep the frozen native bug observable, rather than changing its source.
     * Retail aliasing matched the rehit field; native pointer width shifted the
     * common alias onto the following resume timer. Both actual bodies run. */
    CHECK(offsetof(Fighter,mv.co.common.x1C)-offsetof(Fighter,mv)==32);
    CHECK(offsetof(Fighter,mv.sk.specials.x1C)-offsetof(Fighter,mv)==28);
    CHECK(offsetof(Fighter,mv.sk.specials.x20)-offsetof(Fighter,mv)==32);
    count=3;attributes.x0=3;
    for(unsigned p=0;p<sizeof(parameter_names)/sizeof(*parameter_names);++p)if(!strcmp(parameter_names[p],"It_Kind_Seak_Chain.link_count"))script_parameters[p].i=count;
    reset(false,0,1);it_802BC080_native64(links,&anchor,&item);CHECK(item.xDD4_itemVar.seakchain.x18==0);
    reset(false,0,1);it_802BC080(links,&anchor,&item);CHECK(item.xDD4_itemVar.seakchain.x18==3);
    reset(true,0,1);chain_Control(&object);CHECK(get_int("previous_rehit")==3);
    printf("native64 offsets common_rehit_alias=32 sheik_rehit=28 sheik_resume=32; old_observation=0 corrected_and_authored=3\n");
    const unsigned counts[]={3,6,12,20,30};
    for(unsigned c=0;c<5;++c){count=counts[c];attributes.x0=count;for(unsigned p=0;p<sizeof(parameter_names)/sizeof(*parameter_names);++p)if(!strcmp(parameter_names[p],"It_Kind_Seak_Chain.link_count"))script_parameters[p].i=count;
        for(unsigned first=0;first<count;++first)for(unsigned scenario=0;scenario<16;++scenario)for(contact_mode=0;contact_mode<4;++contact_mode)for(int operation=0;operation<4;++operation){
            if(operation==2&&first==count-1)continue; /* Original final env_flags is undefined for the empty propagation loop. */
            int expected=0;
            for(int generated=0;generated<2;++generated){reset(generated,first,scenario);int actual=0;
                if(operation<2)actual=generated?chain_Extend(&object,operation==1):(operation==1?it_802BBED0(&links[count-1],&anchor,&attributes):it_802BBD64(&links[count-1],&anchor,&attributes));
                else if(operation==2){if(generated)chain_Control(&object);else it_802BC080(links,&anchor,&item);}
                else actual=generated?chain_Retract(&object):it_802BC94C(links,&anchor,&attributes,attributes.x54);
                if(generated){CHECK(expected==actual);compare(operation==2);}else{expected=actual;save();}
            }
        }
    }
    printf("PASS %u article solver pairs, %u checks, maximum_float_difference %.9g\n",pairs,checks,maximum_difference);return 0;
}
