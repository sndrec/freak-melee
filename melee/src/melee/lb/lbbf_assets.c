#include "lbbf_assets.h"

#include "lbcolanim_data.h"

#include <string.h>

bool lbBfAssets_Import(const void* bytes, size_t size, LbBfAssets* output)
{
    u32 root;
    if (output == NULL) {
        return false;
    }
    memset(output, 0, sizeof(*output));
    if (HSD_DatOpen(&output->dat, bytes, size) != HSD_DAT_OK ||
        !HSD_DatFindRoot(&output->dat, "lbBgFlashColAnimData", &root) ||
        HSD_DatNextTarget(&output->dat, root) - root !=
            LB_BF_ENTRY_COUNT * 8)
    {
        output->status = HSD_DAT_SYMBOL;
        return false;
    }
    LbColorScriptTable table = {
        root, LB_BF_ENTRY_COUNT, output->entries,
    };
    return lbColorScript_Import(
        &output->dat, &table, 1, root, output->commands,
        LB_BF_COMMAND_CAPACITY, &output->command_count,
        &output->status, &output->error_offset);
}
