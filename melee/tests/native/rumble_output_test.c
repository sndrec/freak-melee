#include <stdio.h>
#include <stdlib.h>

#include <sysdolphin/baselib/rumble_output.h>

static s32 last_port = -1;
static u32 last_command;
static u32 command_count;

void PADControlMotor(s32 port, u32 command)
{
    last_port = port;
    last_command = command;
    command_count += 1;
}

static void require(bool result, const char* message)
{
    if (!result) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

int main(void)
{
    HSD_RumbleOutputCommand(2, 1);
    require(command_count == 1 && last_port == 2 && last_command == 1,
            "normal rumble command reaches the host controller");

    HSD_RumbleOutputSetSuppressed(true);
    HSD_RumbleOutputCommand(0, 2);
    HSD_RumbleOutputCommand(3, 0);
    require(command_count == 1,
            "hidden resimulation does not issue host motor commands");

    HSD_RumbleOutputSetSuppressed(false);
    HSD_RumbleOutputCommand(1, 2);
    require(command_count == 2 && last_port == 1 && last_command == 2,
            "corrected endpoint command reaches the host controller");

    const s8 shared_pc[4] = { 0, 2, -1, -1 };
    require(HSD_RumbleOutputSetPortMapping(shared_pc),
            "nonconsecutive local controllers form a valid output route");
    require(command_count == 5 && last_port == 3 && last_command == 2,
            "discarded controller routes stop their previous motors");
    u32 before = command_count;
    HSD_RumbleOutputCommand(1, 1);
    require(command_count == before + 1 && last_port == 2 &&
                last_command == 1,
            "the second logical fighter rumbles its physical port 3");
    before = command_count;
    HSD_RumbleOutputCommand(2, 1);
    HSD_RumbleOutputCommand(3, 1);
    HSD_RumbleOutputCommand(-1, 1);
    HSD_RumbleOutputCommand(4, 1);
    require(command_count == before, "remote and invalid ports stay silent");

    const s8 duplicate[4] = { 0, 0, -1, -1 };
    require(!HSD_RumbleOutputSetPortMapping(duplicate),
            "one physical controller cannot serve two fighter slots");
    HSD_RumbleOutputCommand(1, 2);
    require(last_port == 2 && last_command == 2,
            "rejected mappings leave the active route intact");

    const s8 identity[4] = { 0, 1, 2, 3 };
    require(HSD_RumbleOutputSetPortMapping(identity),
            "offline control restores physical port numbering");
    HSD_RumbleOutputCommand(1, 1);
    require(last_port == 1 && last_command == 1,
            "offline rumble again reaches the matching port");
    return 0;
}
