#pragma once

#ifdef __cplusplus
namespace Rml {
class ElementDocument;
}
void nativeShellWorkshop_Init(Rml::ElementDocument* document);
void nativeShellWorkshop_Update();
void nativeShellWorkshop_Close();
bool nativeShellWorkshop_IsOpen();
bool nativeShellWorkshop_IsTesting();
// Rebuild the current draft and restart only the frozen practice preview.
bool nativeShellWorkshop_RefreshPreview();
#endif
