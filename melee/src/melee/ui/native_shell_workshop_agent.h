#pragma once
#include <melee/ft/native_authoring_reply.h>
#include <melee/ft/native_fighter_family_project.h>
namespace Rml { class ElementDocument; }
struct NativeWorkshopLiveState {
    NativeFighterFamilyProject* family = nullptr;
    unsigned member = 0;
    uint64_t generation = 0;
    bool open = false, testing = false, loading = false, allowed = false, dirty = false;
};
NativeWorkshopLiveState nativeShellWorkshop_LiveState();
bool nativeShellWorkshop_SelectMember(unsigned member, bool restart_preview = false);
void nativeWorkshopAgent_Init(Rml::ElementDocument*);
void nativeWorkshopAgent_Shutdown();
bool nativeWorkshopAgent_Click(const char* id);
// Live editor access uses the current member and its existing history.
bool nativeShellWorkshopEditor_AgentState(std::string& fingerprint, NativeAuthoring::Reply* details);
void nativeShellWorkshopEditor_AgentSources(const char* file, NativeAuthoring::Reply&);
void nativeShellWorkshopEditor_AgentSelectSource(const char* file, NativeAuthoring::Reply&);
void nativeShellWorkshopEditor_AgentUpdateSources(yyjson_val*, NativeAuthoring::Reply&);
bool nativeShellWorkshopEditor_AgentUndo(bool redo, NativeAuthoring::Reply&);
bool nativeShellWorkshopEditor_AgentBusy();
void nativeShellWorkshopEditor_AgentWorkspace(const char* workspace);
