#include "native_fighter_action_editor.h"
#include "native_hitbox_lifetime.h"
#include "native_asset_dat.h"
#include <cstdio>
#include <new>

extern "C" bool nativeFighterAction_OwnHitboxFlags(NativeFighterProject* project,
    unsigned* converted,char* error,size_t capacity) try
{
    auto fail=[&](const char* message){if(error&&capacity)std::snprintf(error,capacity,"%s",message);return false;};
    HSD_Dat dat;
    if(!project||!project->action_count||project->action_count>1024||
       HSD_DatOpen(&dat,project->fighter_dat,project->fighter_size)!=HSD_DAT_OK)
        return fail("Hitbox flag ownership needs a valid fighter archive.");
    std::vector<NativeFighterActionCommand> rows;
    struct Change {uint32_t at,value;};std::vector<Change> changes;
    for(unsigned action=0;action<project->action_count;++action){
        size_t count;
        if(!nativeFighterAction_Commands(project,action,nullptr,0,&count))return fail("Could not trace the action's hitbox commands.");
        rows.resize(count);
        if(count&&!nativeFighterAction_Commands(project,action,rows.data(),count,&count))return fail("Could not read the action's hitbox commands.");
        for(const auto& row:rows)if(row.opcode==11){
            u32 raw,following;
            if(!HSD_DatReadU32(&dat,row.source_offset,&raw)||!HSD_DatReadU32(&dat,row.source_offset+20,&following)||((raw>>23)&7)>=4)
                return fail("A retail hitbox has invalid operands or no following flag word.");
            changes.push_back({row.source_offset,(raw&UINT32_C(0x03f7ffff))|(following&(1u<<19))|((uint32_t)NATIVE_HITBOX_EXPLICIT_OPCODE<<26)});
        }
    }
    std::sort(changes.begin(),changes.end(),[](const auto& a,const auto& b){return a.at<b.at;});
    changes.erase(std::unique(changes.begin(),changes.end(),[](const auto& a,const auto& b){return a.at==b.at;}),changes.end());
    for(const auto& change:changes)NativeAssetDat::be32(project->fighter_dat+32+change.at,change.value);
    if(converted)*converted=(unsigned)changes.size();if(error&&capacity)error[0]=0;return true;
}
catch(const std::bad_alloc&)
{if(error&&capacity)std::snprintf(error,capacity,"Could not allocate hitbox flag editing storage.");return false;}
