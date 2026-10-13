#include "rumble.h"

#include <Runtime/platform.h>

/// @todo Circular dependency
#include "controller.h" // IWYU pragma: keep
#ifdef MELEE_NATIVE
#include "rumble_output.h"
#endif
#include <dolphin/os.h>
#include <dolphin/pad.h>

HSD_RumbleData HSD_Rumble_804C22E0[4];

// The native input poll and rumble commands share the game thread.
void HSD_PadRumbleOn(u8 no)
{
    HSD_RumbleData* r5 = &HSD_Rumble_804C22E0[no];

    r5->direct_status = 1;
}

void HSD_PadRumbleOffN(u8 no)
{
    HSD_RumbleData* r5 = &HSD_Rumble_804C22E0[no];

    r5->direct_status = 0;
}

void HSD_PadRumbleFree(HSD_RumbleData* a, HSD_PadRumbleListData* b)
{
    RumbleInfo* r6 = &HSD_PadLibData.rumble_info;
    HSD_PadRumbleListData** r5 = &a->listdatap;

    while ((*r5) != b) {
        r5 = &(*r5)->next;
    }
    *r5 = b->next;
    a->nb_list--;
    b->next = r6->listdatap;
    r6->listdatap = b;
}

void HSD_PadRumbleRemove(u8 no)
{
    HSD_RumbleData* r28 = &HSD_Rumble_804C22E0[no];
    HSD_PadRumbleListData* r4 = r28->listdatap;

    while (r4 != NULL) {
        HSD_PadRumbleListData* r30 = r4->next;
        HSD_PadRumbleFree(r28, r4);
        r4 = r30;
    }
}

void HSD_PadRumbleRemoveAll(void)
{
    int i;

    for (i = 0; i < 4; i++) {
        HSD_PadRumbleRemove(i);
    }
}

void HSD_PadRumbleRemoveId(u8 no, int id)
{
    HSD_RumbleData* r31 = &HSD_Rumble_804C22E0[no];
    HSD_PadRumbleListData* r7 = r31->listdatap;

    while (r7 != NULL) {
        HSD_PadRumbleListData* r6 = r7->next;
        if (r7->id == (unsigned) id) {
            HSD_PadRumbleFree(r31, r7);
        }
        r7 = r6;
    }
}

void HSD_PadRumblePause(u8 no, int status)
{
    HSD_PadRumbleListData* r4 = HSD_Rumble_804C22E0[no].listdatap;

    while (r4 != NULL) {
        HSD_PadRumbleListData* next = r4->next;

        r4->pause = status;
        r4 = next;
    }
}

void HSD_PadRumblePauseAll(void)
{
    u8 _[8];

    int i;
    for (i = 0; i < 4; i++) {
        HSD_PadRumblePause(i, 1);
    }
}

void HSD_PadRumbleUnpauseAll(void)
{
    u8 _[8];

    int i;
    for (i = 0; i < 4; i++) {
        HSD_PadRumblePause(i, 0);
    }
}

void func_80378430_inline(HSD_PadRumbleListData** r6,
                          HSD_PadRumbleListData* r7)
{
    HSD_PadRumbleListData* r5;

    while ((r5 = *r6) != NULL && r5->pri <= r7->pri) {
        r6 = &r5->next;
    }
    r7->next = r5;
    *r6 = r7;
}

int HSD_PadRumbleAdd(u8 no, int id, int frame, int pri, void* listp)
{
    struct RumbleInfo* r31 = &HSD_PadLibData.rumble_info;
    HSD_RumbleData* r30 = &HSD_Rumble_804C22E0[no];
    int r29 = 0;
    HSD_PadRumbleListData* r7 = r31->listdatap;

    if (r7 != NULL && r30->nb_list < r31->max_list) {
        r31->listdatap = r7->next;
        r7->id = id;
        r7->pause = 0;
        r7->pri = pri;
        r7->status = 0;
        r7->loop_count = 0;
        r7->wait = 0;
        r7->frame = frame;
        r7->stack = NULL;
        r7->headp = listp;
        r7->listp = listp;
        func_80378430_inline(&r30->listdatap, r7);
        r30->nb_list++;
        r29 = 1;
    }
    return r29;
}

void HSD_Rumble_80378524(int a)
{

    HSD_PadLibData.rumble_info.unk2 = a;
}

int HSD_PadRumbleInterpret1(HSD_PadRumbleListData* a, u8* b)
{
    if (a->pause == 1) {
        return 0;
    }
    while (a->wait == 0) {
        switch (*a->listp >> 13) {
        case 0:
            if (a->frame == -2) {
                return 1;
            }
            a->listp = a->headp;
            break;
        case 1:
            a->status = 2;
            a->wait = *a->listp & 0x1FFF;
            a->listp++;
            break;
        case 2:
            a->status = 1;
            a->wait = *a->listp & 0x1FFF;
            a->listp++;
            break;
        case 3:
            a->status = 0;
            a->wait = *a->listp & 0x1FFF;
            a->listp++;
            break;
        case 4:
            a->loop_count = *a->listp & 0x1FFF;
            a->listp++;
            a->stack = a->listp;
            break;
        case 5:
            if (--a->loop_count != 0) {
                a->listp = a->stack;
            } else {
                a->listp++;
            }
            break;
        }
    }
    *b = a->status;
    a->wait--;
    if (a->frame != -1 && a->frame != -2) {
        if (--a->frame == 0) {
            return 1;
        }
    }
    return 0;
}

void HSD_PadRumbleInterpret(void)
{
    struct RumbleInfo* r31 = &HSD_PadLibData.rumble_info;
    HSD_RumbleData* r30 = HSD_Rumble_804C22E0;
    HSD_PadRumbleListData* r29;
    HSD_PadRumbleListData* r28;

    int i;
    for (i = 0; i < 4; i++, r30++) {
        r30->status = 0;
        if (r31->unk2 == 0) {
            r30->status = r30->direct_status;
            r29 = r30->listdatap;
            while (r29 != NULL) {
                r28 = r29->next;

                if (HSD_PadRumbleInterpret1((void*) r29, &r30->status) != 0) {
                    HSD_PadRumbleFree(r30, r29);
                }
                r29 = r28;
            }
        }
        if (r30->status != r30->last_status) {
            switch (r30->status) {
            case 0:
#ifdef MELEE_NATIVE
                HSD_RumbleOutputCommand(i, PAD_MOTOR_STOP_HARD);
#else
                PADControlMotor(i, 2);
#endif
                break;
            case 1:
#ifdef MELEE_NATIVE
                HSD_RumbleOutputCommand(i, PAD_MOTOR_STOP);
#else
                PADControlMotor(i, 0);
#endif
                break;
            case 2:
#ifdef MELEE_NATIVE
                HSD_RumbleOutputCommand(i, PAD_MOTOR_RUMBLE);
#else
                PADControlMotor(i, 1);
#endif
                break;
            }
            r30->last_status = r30->status;
        }
    }
}

#ifdef MELEE_NATIVE
void HSD_PadRumbleBeginResimulation(void)
{
    HSD_RumbleOutputSetSuppressed(true);
}

void HSD_PadRumbleEndResimulation(u8 local_port_mask)
{
    HSD_RumbleOutputSetSuppressed(false);
    for (s32 port = 0; port < 4; ++port) {
        if ((local_port_mask & (1U << port)) == 0) {
            continue;
        }
        u32 command;
        switch (HSD_Rumble_804C22E0[port].last_status) {
        case 1:
            command = PAD_MOTOR_STOP;
            break;
        case 2:
            command = PAD_MOTOR_RUMBLE;
            break;
        default:
            command = PAD_MOTOR_STOP_HARD;
            break;
        }
        HSD_RumbleOutputCommand(port, command);
    }
}
#endif

struct HSD_RumbleData HSD_Rumble_80406DE0 = { 0 };

void HSD_PadRumbleInit(u16 a, void* b)
{
    struct RumbleInfo* r6 = &HSD_PadLibData.rumble_info;
    int i;

    r6->unk2 = 0;
    r6->max_list = a;
    r6->listdatap = b;
    if (a != 0) {
        for (i = 0; i < a - 1; i++) {
            r6->listdatap[i].next = &r6->listdatap[i + 1];
        }
        r6->listdatap[i].next = 0;
    }
    for (i = 0; i < 4; i++) {
        HSD_Rumble_804C22E0[i] = HSD_Rumble_80406DE0;
    }
}
