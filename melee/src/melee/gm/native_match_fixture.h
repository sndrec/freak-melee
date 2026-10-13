#ifndef MELEE_GM_NATIVE_MATCH_FIXTURE_H
#define MELEE_GM_NATIVE_MATCH_FIXTURE_H
#ifdef MELEE_NATIVE
#include <stdbool.h>
#include <stdint.h>

// Offline input-replay fixtures. Never active in normal play or netplay.
void nativeMatchFixture_Reset(void);
bool nativeMatchFixture_AddItem(const char* specification, bool capture_eligible);
bool nativeMatchFixture_AddAirborne(const char* specification);
bool nativeMatchFixture_Configured(void);
bool nativeMatchFixture_Apply(uint64_t sample);
#endif
#endif
