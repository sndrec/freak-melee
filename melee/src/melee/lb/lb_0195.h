#ifndef GALE01_01955C
#define GALE01_01955C

#include <Runtime/platform.h>

/* 01955C */ void lb_8001955C(void);
/* 0195D0 */ void lb_800195D0(void);
void lbInput_SetGameSpeed(float speed);
// Called on the game thread with a monotonic nanosecond timestamp.
u8 lbInput_Poll(u64 now_ns);
bool lbInput_DidSample(void);
u64 lbInput_VirtualClockNowNs(void);
// Replay one scheduled sample without waiting on present-day wall time.
u64 lbInput_NextSampleTimeNs(void);
// Start a scene's host sampling schedule at the current monotonic time.
// Simulation phase and game speed remain unchanged.
void lbInput_RebaseClock(u64 now_ns);
/* 0198E0 */ void lb_800198E0(void);
/* 019900 */ void lb_80019900(void);
/* 019A30 */ bool lb_80019A30(int);
/* 019A48 */ void lb_80019A48(void);
/* 019AAC */ void lb_80019AAC(Event);

#endif
