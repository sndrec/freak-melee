#include "dat_audio.h"

enum { AUDIO_SILENCE = 0x83D60 };

bool HSD_DatAudioLoadLayout(const HSD_Dat* dat, u32 root,
                            u32* value_count)
{
    u32 table_offsets[HSD_AUDIO_LOAD_TABLES];
    const char* external;
    if (!value_count || dat->root_count != 1 || dat->extern_count != 0 ||
        root > dat->data_size || dat->data_size - root < 16) {
        return false;
    }
    for (u32 language = 0; language < HSD_AUDIO_LOAD_TABLES; ++language) {
        if (HSD_DatResolve(dat, root + language * 4,
                           &table_offsets[language], &external) !=
            HSD_DAT_REF_LOCAL) {
            return false;
        }
    }
    const u32 first_table = table_offsets[0];
    const u32 table_size = HSD_AUDIO_LOAD_LISTS * 4;
    if (first_table % 4 || root != first_table +
            HSD_AUDIO_LOAD_TABLES * table_size) {
        return false;
    }
    for (u32 language = 0; language < HSD_AUDIO_LOAD_TABLES; ++language) {
        if (table_offsets[language] != first_table + language * table_size) {
            return false;
        }
        for (u32 list = 0; list < HSD_AUDIO_LOAD_LISTS; ++list) {
            u32 target;
            if (HSD_DatResolve(dat, table_offsets[language] + list * 4,
                               &target, &external) != HSD_DAT_REF_LOCAL ||
                target % 4 || target >= first_table) {
                return false;
            }
            bool terminated = false;
            for (u32 offset = target; offset < first_table; offset += 4) {
                u32 value;
                if (!HSD_DatReadU32(dat, offset, &value)) return false;
                if (value == AUDIO_SILENCE) {
                    terminated = true;
                    break;
                }
            }
            if (!terminated) return false;
        }
    }
    *value_count = first_table / 4;
    return true;
}

bool HSD_DatImportAudioLoad(const HSD_Dat* dat, u32 root,
                            HSD_AudioLoadData* output,
                            u32** list_storage, size_t list_capacity,
                            u32* value_storage, size_t value_capacity)
{
    u32 value_count;
    const size_t list_count = HSD_AUDIO_LOAD_TABLES * HSD_AUDIO_LOAD_LISTS;
    if (!output || !list_storage || !value_storage ||
        !HSD_DatAudioLoadLayout(dat, root, &value_count) ||
        list_capacity < list_count || value_capacity < value_count) {
        return false;
    }
    for (u32 i = 0; i < value_count; ++i) {
        HSD_DatReadU32(dat, i * 4, &value_storage[i]);
    }
    const u32 first_table = value_count * 4;
    const char* external;
    for (u32 language = 0; language < HSD_AUDIO_LOAD_TABLES; ++language) {
        output->tables[language] =
            list_storage + language * HSD_AUDIO_LOAD_LISTS;
        for (u32 list = 0; list < HSD_AUDIO_LOAD_LISTS; ++list) {
            u32 target;
            HSD_DatResolve(dat, first_table +
                               (language * HSD_AUDIO_LOAD_LISTS + list) * 4,
                           &target, &external);
            output->tables[language][list] = value_storage + target / 4;
        }
    }
    return true;
}
