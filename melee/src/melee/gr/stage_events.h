#ifndef MELEE_GR_STAGE_EVENTS_H
#define MELEE_GR_STAGE_EVENTS_H
#include <Runtime/platform.h>
// FNV-1a hashes of the provider's public channel names.
#define StageEvent_Conversation_available 4162349027U
#define StageEvent_Conversation_active 3032060146U
#define StageEvent_Conversation_start 3016788868U
#define StageEvent_Conversation_cancel 2805652530U
bool Stage_Event(u32 channel, float value);
#endif
