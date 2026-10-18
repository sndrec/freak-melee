#include <melee/gm/native_match_selection.h>
#include <melee/gm/native_stock_content.h>
#include <melee/gm/gmmain_lib.h>
#include <melee/gm/types.h>
#include <melee/mn/types.h>
#include <melee/ft/forward.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static GameRules rules = {.mode=NativeLobbyMatchMode_Stock, .stock_count=1, .damage_ratio=10};
static struct GamePrefs prefs = {.item_freq=UINT8_MAX};
GameRules* gmMainLib_GetGameRules(void) { return &rules; }
struct GamePrefs* gmMainLib_GetGamePrefs(void) { return &prefs; }
u8 gm_GetNumCostumesForCKind(u8 character) { (void)character; return 4; }

int main(void)
{
    VsModeData vs = {0};
    NativeContentId fighters[4] = {0}, costumes[4] = {0};
    for (unsigned port=0; port<4; ++port) {
        vs.start.players[port].slot_type=Gm_PKind_Human;
        vs.start.players[port].ckind=ChKind_CustomFirst+port;
        vs.start.players[port].handicap=9;
        fighters[port].bytes[0]=port+1;
    }
    NativeLobbyMatch match;
    for (unsigned mask=0; mask<16; ++mask) {
        memset(costumes,0,sizeof(costumes));
        for(unsigned port=0;port<4;++port)
            if(mask&(1U<<port))costumes[port].bytes[0]=port+10;
        assert(nativeMatchSelection_Build(&match,&vs,32,costumes,fighters));
        for(unsigned port=0;port<4;++port) {
            assert(!memcmp(&match.slots[port].character_package_id,&fighters[port],sizeof(fighters[port])));
            assert(!memcmp(&match.slots[port].costume_package_id,&costumes[port],sizeof(costumes[port])));
            assert(!match.slots[port].character_id&&!match.slots[port].costume_id);
        }
    }
    vs.start.players[0].color=1;
    assert(!nativeMatchSelection_Build(&match,&vs,32,costumes,fighters));
    vs.start.players[0].color=0;
    vs.start.players[0].ckind=CKind_Mario;
    assert(!nativeMatchSelection_Build(&match,&vs,32,costumes,fighters));
    fighters[0]=(NativeContentId){0};
    vs.start.players[0].color=3;
    assert(nativeMatchSelection_Build(&match,&vs,32,costumes,fighters));
    assert(match.slots[0].character_id==nativeStockContent_CharacterId(CKind_Mario));
    assert(match.slots[0].costume_id==nativeStockContent_CostumeId(CKind_Mario,3));
    assert(!memcmp(&match.slots[0].costume_package_id,&costumes[0],sizeof(costumes[0])));
    vs.start.players[0].color=4;
    assert(!nativeMatchSelection_Build(&match,&vs,32,costumes,fighters));
    puts("PASS custom outfits across all four match slots and mixed stock/custom selection");
    return 0;
}
