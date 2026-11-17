#pragma once

#ifdef __cplusplus
#include <string>
namespace NativeAuthoring { struct Reply; }
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
// Build a cached snapshot and enter frozen preview or normal practice.
bool nativeShellWorkshop_RequestPreview(bool live_test, NativeAuthoring::Reply* = nullptr);
bool nativeShellWorkshop_StopTest(NativeAuthoring::Reply&);
bool nativeShellWorkshop_Build(NativeAuthoring::Reply&);
bool nativeShellWorkshop_Install(NativeAuthoring::Reply&);
void nativeShellWorkshop_Artifacts(NativeAuthoring::Reply&);
bool nativeShellWorkshop_Save(std::string* error = nullptr);
bool nativeShellWorkshop_Check(NativeAuthoring::Reply&);
#endif
