// These unrelated operations are not exercised by this observation fixture.
// Abort rather than supply a fabricated successful result if it reaches them.
#include <melee/ft/ftparts.h>
#include <melee/mp/mplib.h>
#include <sysdolphin/baselib/mtx.h>
#include <stdio.h>
#include <stdlib.h>

FighterPartsTable** ftPartsTable;

void HSD_MtxGetRotation(Mtx matrix, Vec3* output)
{
    (void)matrix; (void)output;
    fputs("Unselected attachment-matrix operation reached\n", stderr);
    abort();
}

float mpLib_800569EC(u32 material)
{
    (void)material;
    fputs("Unselected floor-material operation reached\n", stderr);
    abort();
}
