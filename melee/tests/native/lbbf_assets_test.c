#include <stdint.h>

#include <aurora/dvd.h>
#include <dolphin/dvd.h>
#include <melee/lb/lbbf_assets.h>
#include <melee/lb/lb_013B.h>
#include <melee/lb/lbcommand.h>

#include <stdio.h>
#include <stdlib.h>

static void require(bool condition, const char* message)
{
    if (!condition) {
        fputs(message, stderr);
        exit(1);
    }
}

static void unused_color_callback(Fighter_GObj* gobj, CommandInfo* command,
                                  int opcode)
{
    require(false, "unexpected flash callback\n");
}

int main(int argc, char** argv)
{
    DVDFileInfo file;
    require(argc == 2, "usage: native_lbbf_assets_test <iso>\n");
    require(aurora_dvd_open(argv[1]) && DVDOpen("/LbBf.dat", &file),
            "open LbBf.dat\n");
    size_t size = file.length;
    size_t padded = (size + 31) & ~(size_t) 31;
    void* bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= (s32) size &&
                DVDClose(&file),
            "read LbBf.dat\n");
    LbBfAssets assets;
    if (!lbBfAssets_Import(bytes, size, &assets)) {
        fprintf(stderr, "flash import failed: status=%d offset=0x%lX\n",
                assets.status, assets.error_offset);
        return 1;
    }
    require(assets.entries[0].unk == NULL &&
                assets.entries[1].unk != NULL &&
                assets.entries[2].unk != NULL &&
                ((union ColorOverlay_x8_t*) assets.entries[2].unk)->unk.unk ==
                    18 &&
                assets.command_count < LB_BF_COMMAND_CAPACITY,
            "decoded flash scripts and opcode bitfields\n");
    CommandInfo loop = { 0 };
    loop.u = assets.entries[11].unk;
    require(loop.u->Command_03.code == 3 &&
                loop.u->Command_03.value == 10,
            "decoded flash loop command\n");
    Command_03(&loop);
    require(loop.loop_count == 2 &&
                (uintptr_t) loop.event_return[1] == 10,
            "native loop setup\n");
    union CmdUnion* body = loop.u;
    for (u32 i = 9; i > 0; --i) {
        Command_04(&loop);
        require(loop.u == body &&
                    (uintptr_t) loop.event_return[1] == i,
                "native loop iteration\n");
    }
    Command_04(&loop);
    require(loop.loop_count == 0 && loop.u == body + 1,
            "native loop completion\n");

    union CmdUnion words[6] = { 0 };
    CommandInfo branch = { 0 };
    require(sizeof(words[0]) == 4,
            "native command word is four bytes\n");
    branch.u = words;
    words[1].Command_05.relative_offset = 2 * sizeof(words[0]);
    Command_05(&branch);
    require(branch.u == words + 3 && branch.loop_count == 1 &&
                branch.event_return[0] == words + 2,
            "native subroutine enters target and saves return\n");
    Command_06(&branch);
    require(branch.u == words + 2 && branch.loop_count == 0,
            "native subroutine returns to next command\n");
    branch.u = words + 3;
    words[4].Command_07.relative_offset = -3 * (s32) sizeof(words[0]);
    Command_07(&branch);
    require(branch.u == words + 1,
            "native goto follows four-byte relative operand\n");
    branch.u = words + 3;
    words[4].Command_07.relative_offset = INT32_MIN;
    Command_07(&branch);
    require(branch.u == NULL,
            "native null goto terminates the script\n");

    for (u32 entry = 1; entry < LB_BF_ENTRY_COUNT; ++entry) {
        ColorOverlay overlay = { 0 };
        lb_80014498(&overlay);
        require(lb_800144C8(&overlay, assets.entries, entry, 0),
                "start retail flash script\n");
        bool completed = false;
        for (u32 frame = 0; frame < 1000; ++frame) {
            if (lb_80014258(NULL, &overlay, unused_color_callback)) {
                completed = true;
                break;
            }
        }
        require(completed, "retail flash script reaches its terminator\n");
    }
    printf("entries=%u commands=%lu\n", LB_BF_ENTRY_COUNT,
           assets.command_count);
    _aligned_free(bytes);
    aurora_dvd_close();
    return 0;
}
