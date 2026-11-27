#ifndef MELEE_UI_NATIVE_SHELL_WORKSHOP_EDITOR_H
#define MELEE_UI_NATIVE_SHELL_WORKSHOP_EDITOR_H

#ifdef MELEE_NATIVE

#include <string>

namespace Rml { class ElementDocument; }
struct NativeFighterProject;
struct NativeFighterModelEditor;
namespace NativeFighterSources { struct Session; }
namespace NativeAuthoring { struct Reply; }
struct yyjson_val;

void nativeShellWorkshopEditor_RegisterGraph();
void nativeShellWorkshopEditor_Actions(const char* operation, yyjson_val*, NativeAuthoring::Reply&);
void nativeShellWorkshopEditor_Animations(const char* operation, yyjson_val*, NativeAuthoring::Reply&);
void nativeShellWorkshopEditor_Attributes(const char* operation, yyjson_val*, NativeAuthoring::Reply&);
void nativeShellWorkshopEditor_Resources(const char* operation, yyjson_val*, NativeAuthoring::Reply&);
void nativeShellWorkshopEditor_Init(Rml::ElementDocument* document);
void nativeShellWorkshopEditor_Open(unsigned member, NativeFighterProject* project,
                                    const char* path);
bool nativeShellWorkshopEditor_Select(unsigned member);
void nativeShellWorkshopEditor_Close();
bool nativeShellWorkshopEditor_Click(const char* id, bool shift = false);
void nativeShellWorkshopEditor_FilterSounds();
void nativeShellWorkshopEditor_ApplyExecuteFunction(const char* function);
bool nativeShellWorkshopEditor_ScrubTimeline(const char* id, int mouse_x);
bool nativeShellWorkshopEditor_Shortcut(int key_identifier);
void nativeShellWorkshopEditor_FilterActions();
void nativeShellWorkshopEditor_FilterClips();
void nativeShellWorkshopEditor_RefreshScriptOutline();
void nativeShellWorkshopEditor_Update();
NativeFighterSources::Session* nativeShellWorkshopEditor_CaptureSources();
NativeFighterSources::Session* nativeShellWorkshopEditor_SourceSession(unsigned member);
void nativeShellWorkshopEditor_PresentValidation(NativeAuthoring::Reply&);
bool nativeShellWorkshopEditor_HasUnsavedEdits(unsigned member);
void nativeShellWorkshopEditor_MarkAssetsSaved();
bool nativeShellWorkshopEditor_ScriptChanged(unsigned member);
bool nativeShellWorkshopEditor_PreviewSource(unsigned member, std::string* source,
                                             std::string* parameters);
bool nativeShellWorkshopEditor_IsScriptMode();
bool nativeShellWorkshopEditor_IsAnimationMode();
bool nativeShellWorkshopEditor_IsModelMode();
bool nativeShellWorkshopEditor_IsGraphFocused();
float nativeShellWorkshopEditor_TimelineFraction();
bool nativeShellWorkshopEditor_IsOpen();
void nativeShellWorkshopEditor_SetTesting(bool testing);
void nativeShellWorkshopEditor_RigChanged(const NativeFighterModelEditor& before,
                                         const NativeFighterModelEditor& after);

#endif
#endif
