#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftdat.h>
#include <melee/ft/types.h>
#include <melee/lb/lbaudio_ax.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    static const struct {
        FighterKind fighter;
        CharacterKind character;
    } sources[] = {
        { Ft_Kind_Mario, CKind_Mario }, { Ft_Kind_DrMario, CKind_DrMario },
        { Ft_Kind_Captain, CKind_Captain }, { Ft_Kind_Ganon, CKind_Ganon },
        { Ft_Kind_Luigi, CKind_Luigi }, { Ft_Kind_Purin, CKind_Purin },
        { Ft_Kind_Fox, CKind_Fox }, { Ft_Kind_Falco, CKind_Falco },
        { Ft_Kind_Mars, CKind_Mars }, { Ft_Kind_Emblem, CKind_Emblem },
        { Ft_Kind_Donkey, CKind_Donkey }, { Ft_Kind_Koopa, CKind_Koopa },
        { Ft_Kind_Pikachu, CKind_Pikachu }, { Ft_Kind_Pichu, CKind_Pichu },
        { Ft_Kind_Samus, CKind_Samus },
        { Ft_Kind_Link, CKind_Link }, { Ft_Kind_CLink, CKind_CLink },
        { Ft_Kind_Mewtwo, CKind_Mewtwo }, { Ft_Kind_Yoshi, CKind_Yoshi },
        { Ft_Kind_Ness, CKind_Ness },
        { Ft_Kind_Peach, CKind_Peach },
        { Ft_Kind_GameWatch, CKind_GameWatch },
        { Ft_Kind_Zelda, CKind_Zelda }, { Ft_Kind_Seak, CKind_Seak },
        { Ft_Kind_Popo, CKind_PopoNana }, { Ft_Kind_Nana, CKind_PopoNana },
    };
    for (unsigned i = 0; i < sizeof(sources) / sizeof(sources[0]); ++i) {
        const NativeFighterDefinition* base =
            nativeFighterRuntime_FindStockDefinition(sources[i].fighter);
        if (base == NULL ||
            base->action_count != ftData_Table_Unk0[sources[i].fighter].count ||
            base->demo_count != ftData_UnkIntPairs[sources[i].fighter].count ||
            base->sound_banks != lbAudioAx_80026E84(sources[i].character)) {
            fprintf(stderr, "Clone metadata differs from stock fighter %d\n",
                    sources[i].fighter);
            return 1;
        }
        const Fighter_DemoStrings* copied = base->demo_motion_names;
        const Fighter_DemoStrings* stock = ftData_803C2468[sources[i].fighter];
        if (copied == NULL || copied == stock || stock == NULL ||
            strcmp(copied->result_filename, stock->result_filename) != 0 ||
            strcmp(copied->intro_filename, stock->intro_filename) != 0 ||
            strcmp(copied->ending_filename, stock->ending_filename) != 0 ||
            strcmp(copied->vi_wait_filename, stock->vi_wait_filename) != 0) {
            fprintf(stderr, "Clone demo roots differ from stock fighter %d\n",
                    sources[i].fighter);
            return 1;
        }
        if (sources[i].fighter == Ft_Kind_Link || sources[i].fighter == Ft_Kind_CLink) {
            bool young = sources[i].fighter == Ft_Kind_CLink;
            unsigned physical = young ? 79 : 75;
            if (base->joint_count != physical || base->bone_count != 53 ||
                base->bones == NULL || base->omission_count != 1 ||
                base->omissions == NULL || base->article_asset_count != (young ? 6 : 5)) return 1;
            const NativeFighterPartOmission* omission = base->omissions;
            if (omission->joint != (young ? 72 : 68) || omission->parent != (young ? 71 : 67) ||
                omission->insertion_mode != 0 || omission->donor_joint != 255) return 1;
            bool seen[FT_COMMON_PART_COUNT] = {0};
            for (unsigned n = 0; n < base->bone_count; ++n) {
                const NativeFighterBone* bone = &base->bones[n];
                if ((unsigned)bone->part >= FT_COMMON_PART_COUNT || seen[bone->part] ||
                    bone->joint >= physical + 1 || bone->part == FtPart_WaistN) return 1;
                seen[bone->part] = true;
                if (bone->part == 53 && bone->joint != physical) return 1;
            }
            const ItemKind articles[] = {
                young ? It_Kind_CLink_Bomb : It_Kind_Link_Bomb,
                young ? It_Kind_CLink_Boomerang : It_Kind_Link_Boomerang,
                young ? It_Kind_CLink_HShot : It_Kind_Link_HShot,
                young ? It_Kind_CLink_Arrow : It_Kind_Link_Arrow,
                young ? It_Kind_CLink_Bow : It_Kind_Link_Bow,
                It_Kind_CLink_Milk,
            };
            const int offsets[] = {72,44,188,12,16,-1};
            for (unsigned n = 0; n < base->article_asset_count; ++n)
                if (base->article_assets[n].kind != articles[n] || base->article_assets[n].slot != n ||
                    base->article_assets[n].kind_attribute_offset != offsets[n]) return 1;
        }
    }
    return 0;
}
