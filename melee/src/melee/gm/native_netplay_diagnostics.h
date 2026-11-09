#ifndef MELEE_GM_NATIVE_NETPLAY_DIAGNOSTICS_H
#define MELEE_GM_NATIVE_NETPLAY_DIAGNOSTICS_H

#include <stdint.h>
#include <stdio.h>

FILE* nativeNetplayDiagnostics_Open(uint64_t session, unsigned peer);
void nativeNetplayDiagnostics_Send(FILE* report, uint64_t session, unsigned peer);

#endif
