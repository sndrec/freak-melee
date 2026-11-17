#pragma once

#ifdef __cplusplus
#include <string>
extern "C" {
#else
#include <stdbool.h>
#endif
bool nativeShellWorkshop_ImportFighter(const char* package_path);
bool nativeShellWorkshop_ImportHudImage(unsigned target, const char* png_path);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
namespace Rml {
class ElementDocument;
}
void nativeShellWorkshop_Init(Rml::ElementDocument* document);
enum class NativeWorkshopOrigin { Library,Collection,Hub };
void nativeShellWorkshop_Open(const char* draft_path = nullptr,
                               NativeWorkshopOrigin origin = NativeWorkshopOrigin::Library);
void nativeShellWorkshop_Update();
void nativeShellWorkshop_Close();
bool nativeShellWorkshop_IsOpen();
bool nativeShellWorkshop_IsTesting();
// Rebuild the current draft and restart only the frozen practice preview.
bool nativeShellWorkshop_RefreshPreview();
bool nativeShellWorkshop_Save(std::string* error = nullptr);
#endif
