#include <melee/ft/kinds/ftCommon/ftCo_Throw.h>
#include "throw_presentation_reference.inc"

static unsigned throw_presentation_cases_count;
static void publish_presentation_hooks(uint32_t setup,uint32_t presentation)
{
    capture_package.program.hooks[NativeScriptFighter_ThrowVictimSetup]=setup;
    capture_package.program.hooks[NativeScriptFighter_ThrowPresentation]=presentation;
    CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&capture_package.program);
}
static Fighter_GObj* presentation_pair(bool owned)
{
    reset();capture_motion_case=throw_entry_case=true;
    Fighter_GObj* h=create(true);candidate=create(owned);reciprocal_pair(h,candidate);
    Fighter* holder=h->user_data;Fighter* victim=candidate->user_data;
    holder->cmd_vars[0]=5;holder->throw_flags=255;holder->mv.co.throw.x4=7;holder->mv.co.throw.x8=9;
    holder->throw_invincibility_frames=2;holder->x198C=0;
    victim->grab_timer=17;victim->x1A6A=0;retirement_holder=h;
    return h;
}
static void throw_presentation_cases(void)
{
    ftCommonData common={0};common.x348=20;
    ftCommonData* previous=p_ftCommonData;p_ftCommonData=&common;
    const uint32_t setup=capture_package.program.hooks[NativeScriptFighter_ThrowVictimSetup];
    const uint32_t presentation=capture_package.program.hooks[NativeScriptFighter_ThrowPresentation];
    CHECK(presentation!=UINT32_MAX);
    publish_presentation_hooks(UINT32_MAX,presentation);
    const float rates[]={0.75f,1.0f,1.8f};
    for(unsigned owned=0;owned<2;++owned)for(unsigned motion=0;motion<4;++motion)for(unsigned rate=0;rate<3;++rate){
        Fighter_GObj* h=presentation_pair(owned);Fighter_GObj* v=candidate;
        Fighter* holder=h->user_data;holder->x1990=motion&1;throw_presentation_probe=true;
        ftCo_800DD398(h,(FtMotionId)(ftCo_MS_ThrowF+motion),(FtMotionId)(ftCo_MS_ThrownF+motion),rates[rate]);
        CHECK(throw_presentation_observations==1&&throw_presentation_invincibility==1&&!throw_presentation_stock_accessories);
        CHECK(trace_count>=2&&trace[trace_count-2]==23&&trace[trace_count-1]==24);
        CHECK(holder->cmd_vars[0]==0&&holder->throw_flags==0&&holder->mv.co.throw.x4==0&&holder->mv.co.throw.x8==0);
        CHECK(holder->throw_invincibility_frames==20&&holder->x198C==((motion&1)?2:1));
        CHECK(presentation_state(h,"items",NULL).as.i64==ftCo_MS_ThrowF+motion);
        CHECK(presentation_state(h,"victim_frame",NULL).as.f32==rates[rate]);
        CHECK(presentation_state(h,"victims",NULL).as.i64==1&&holder->cur_pos.x==2);
        HSD_GObjFree(v);HSD_GObjFree(h);++cases;++throw_presentation_cases_count;
    }
    // Missing authored hook is a healthy no-op, with the common continuation.
    // It does not fall back to any native fighter-specific constructor.
    publish_presentation_hooks(UINT32_MAX,UINT32_MAX);
    Fighter_GObj* h=presentation_pair(true);Fighter_GObj* v=candidate;
    ftCo_800DD398(h,ftCo_MS_ThrowF,ftCo_MS_ThrownF,1);
    CHECK(!throw_presentation_observations&&!throw_presentation_stock_accessories&&throw_presentation_invincibility==1);
    CHECK(presentation_state(h,"victims",NULL).as.i64==0&&((Fighter*)h->user_data)->cur_pos.x==0);
    HSD_GObjFree(v);HSD_GObjFree(h);++cases;++throw_presentation_cases_count;
    publish_presentation_hooks(UINT32_MAX,presentation);

    // A callback can fault/retire its actor or break either capture link.
    // Successful VM return is insufficient to authorize native continuation.
    for(unsigned bad=1;bad<=10;++bad){
        h=presentation_pair(true);v=candidate;throw_presentation_probe=true;
        if(bad<=7){presentation_retire_mode=bad;presentation_retire_at=1;}
        else capture_hook_peer_change=bad-7;
        ftCo_800DD398(h,ftCo_MS_ThrowHi,ftCo_MS_ThrownHi,0.75f);
        CHECK(!throw_presentation_invincibility&&!throw_presentation_stock_accessories);
        presentation_restore();HSD_GObjFree(v);HSD_GObjFree(h);++cases;++throw_presentation_cases_count;
    }
    // Retire either participant at holder motion, holder animation, victim
    // motion and victim animation observer boundaries.
    const unsigned boundaries[][2]={{5,0},{7,0},{4,0},{5,1},{7,1}};
    for(unsigned boundary=0;boundary<5;++boundary)for(unsigned peer=0;peer<2;++peer){
        h=presentation_pair(true);v=candidate;retire_step=boundaries[boundary][0];retire_skip=boundaries[boundary][1];retire_target=peer;
        ftCo_800DD398(h,ftCo_MS_ThrowLw,ftCo_MS_ThrownLw,1);
        CHECK(!throw_presentation_invincibility&&!throw_presentation_observations);
        HSD_GObjFree(v);HSD_GObjFree(h);++cases;++throw_presentation_cases_count;
    }
    // Reject malformed reciprocal links before even clearing the holder's
    // throw commands or entering either fighter's motion.
    for(unsigned bad=0;bad<5;++bad){
        h=presentation_pair(true);v=candidate;Fighter* holder=h->user_data;Fighter* victim=v->user_data;
        if(bad==0)holder->x1A5C=NULL;
        if(bad==1)victim->x1A5C=NULL;
        if(bad==2)++holder->native_hitlag_link_generation;
        if(bad==3)++victim->native_hitlag_link_generation;
        if(bad==4)holder->victim_gobj=NULL;
        Fighter hs=*holder,vs=*victim;
        ftCo_800DD398(h,ftCo_MS_ThrowF,ftCo_MS_ThrownF,1);
        CHECK(!trace_count&&!throw_presentation_invincibility&&!memcmp(holder,&hs,sizeof(hs))&&!memcmp(victim,&vs,sizeof(vs)));
        HSD_GObjFree(v);HSD_GObjFree(h);++cases;++throw_presentation_cases_count;
    }
    // Native holders retain the previous entry behavior. Compare whole fighter
    // bytes and observer order, including Samus's native accessory and Kirby's
    // upward-throw flag, against the exact pre-event entry body.
    const FighterKind kinds[]={Ft_Kind_Mario,Ft_Kind_Samus,Ft_Kind_Kirby};
    for(unsigned kind=0;kind<3;++kind)for(unsigned motion=0;motion<4;++motion){
        reset();capture_motion_case=throw_entry_case=true;h=create(false);v=create(false);candidate=v;reciprocal_pair(h,v);
        Fighter* holder=h->user_data;Fighter* victim=v->user_data;holder->kind=kinds[kind];victim->grab_timer=17;
        Fighter hs=*holder,vs=*victim;reference_throw_entry(h,(FtMotionId)(ftCo_MS_ThrowF+motion),(FtMotionId)(ftCo_MS_ThrownF+motion),0.75f);
        Fighter he=*holder,ve=*victim;unsigned order[32],n=trace_count;memcpy(order,trace,sizeof(order));
        unsigned invincibility=throw_presentation_invincibility,accessories=throw_presentation_stock_accessories;
        *holder=hs;*victim=vs;trace_count=throw_presentation_invincibility=throw_presentation_stock_accessories=0;
        ftCo_800DD398(h,(FtMotionId)(ftCo_MS_ThrowF+motion),(FtMotionId)(ftCo_MS_ThrownF+motion),0.75f);
        CHECK(!memcmp(holder,&he,sizeof(he))&&!memcmp(victim,&ve,sizeof(ve)));
        CHECK(trace_count==n&&!memcmp(trace,order,n*sizeof(*trace)));
        CHECK(throw_presentation_invincibility==invincibility&&throw_presentation_stock_accessories==accessories);
        HSD_GObjFree(v);HSD_GObjFree(h);++cases;++throw_presentation_cases_count;
    }
    publish_presentation_hooks(setup,presentation);
    p_ftCommonData=previous;
}
