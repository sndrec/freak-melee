#pragma once
#ifdef __cplusplus
extern "C" {
#endif
// Retail unlock/milestone scene from IfPrize.usd. Load at hub entry; the
// model retains its authored camera, materials and five-frame entrance.
void nativeMenuPrize_Load(void);
void nativeMenuPrize_Unload(void);
void nativeMenuPrize_Draw(float frame, unsigned category);
#ifdef __cplusplus
}
#endif
