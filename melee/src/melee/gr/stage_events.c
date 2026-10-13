#include "stage_events.h"
#include "types.h"
#include "ground.h"
bool Stage_Event(u32 channel, float value)
{
    return stage_info.on_event != NULL && stage_info.on_event(channel, value);
}
