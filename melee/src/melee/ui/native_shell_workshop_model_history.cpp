#include "native_shell_workshop_model.h"
#include "native_shell_workshop_model_internal.h"
#include "native_shell_workshop.h"
#include "native_shell_workshop_editor.h"
#include <melee/ft/native_fighter_authoring.h>
#include <RmlUi/Core.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace WorkshopModelInternal {
void buttons()
{
    for (auto button : { std::pair<const char*, bool>{ "workshop-undo",
                         active->change_cursor != 0 },
                         { "workshop-redo", active->change_cursor < active->changes.size() } }) {
        if (button.second) element(button.first)->RemoveAttribute("disabled");
        else element(button.first)->SetAttribute("disabled", "");
    }
}

void record(uint32_t offset, unsigned count, const uint8_t before[4])
{
    if (active->project == nullptr || count == 0 || count > 4 ||
        offset > active->project->costume_size ||
        count > active->project->costume_size - offset) return;
    Change change = {};
    change.rig_revision = active->project->rig_edit_revision;
    change.offset = offset;
    change.count = (uint8_t) count;
    std::memcpy(change.before, before, count);
    std::memcpy(change.after, active->project->costume_dat + offset, count);
    if (std::memcmp(change.before, change.after, count) == 0) return;
    active->changes.erase(active->changes.begin() + active->change_cursor, active->changes.end());
    if (active->changes.size() == 64) {
        active->changes.erase(active->changes.begin());
        --active->change_cursor;
    }
    active->changes.push_back(std::move(change));
    active->change_cursor = active->changes.size();
    buttons();
}

void record_asset(const std::vector<uint8_t>& before,const std::vector<uint8_t>& fighter)
{
    if (before.size() == active->project->costume_size &&
        std::memcmp(before.data(), active->project->costume_dat,
                    before.size()) == 0) return;
    Change change = {};
    change.rig_revision = active->project->rig_edit_revision;
    change.before_asset = before;
    change.after_asset.assign(active->project->costume_dat,
                              active->project->costume_dat + active->project->costume_size);
    if(!fighter.empty()) {
        change.before_fighter=fighter;
        change.after_fighter.assign(active->project->fighter_dat,active->project->fighter_dat+active->project->fighter_size);
    }
    active->changes.erase(active->changes.begin() + active->change_cursor, active->changes.end());
    if (active->changes.size() == 64) {
        active->changes.erase(active->changes.begin());
        --active->change_cursor;
    }
    active->changes.push_back(std::move(change));
    active->change_cursor = active->changes.size();
    buttons();
}

std::vector<uint8_t> label_snapshot()
{
    if (!active->project->authoring) {
        return {};
    }
    auto* begin = (uint8_t*) active->project->authoring;
    return std::vector<uint8_t>(begin, begin + sizeof(NativeFighterAuthoring));
}
void record_label_change(const std::vector<uint8_t>& before, bool asset)
{
    if (!asset) {
        active->changes.erase(active->changes.begin() + active->change_cursor,
                              active->changes.end());
        if (active->changes.size() == 64) {
            active->changes.erase(active->changes.begin());
            --active->change_cursor;
        }
        active->changes.push_back(Change{});
        active->change_cursor = active->changes.size();
    }
    auto& change = active->changes.back();
    change.rig_revision = active->project->rig_edit_revision;
    change.labels_saved = true;
    change.before_labels = before;
    change.after_labels = label_snapshot();
    buttons();
}

}
using namespace WorkshopModelInternal;

bool nativeShellWorkshopModel_Undo(bool redo)
{
    if (!visible || active->project == nullptr ||
        (redo ? active->change_cursor == active->changes.size() : active->change_cursor == 0))
        return false;
    const Change& change = active->changes[redo ? active->change_cursor : active->change_cursor - 1];
    if(change.rig) {
        auto before=std::make_unique<NativeFighterModelEditor>(active->model);
        char error[256];
        if(!nativeFighterCoreAssets_Restore(*active->project,
            redo?change.rig->before:change.rig->after,
            redo?change.rig->after:change.rig->before,error,sizeof(error))) {
            status(error);return false;
        }
        if(redo) ++active->change_cursor;else --active->change_cursor;
        playing=false;
        reopen_model(redo?&change.rig->after_view:&change.rig->before_view);
        nativeShellWorkshopEditor_RigChanged(*before,active->model);
        nativeShellWorkshop_RefreshPreview();nativeHome_WorkshopPoseInvalidate();buttons();
        status(redo?"Reapplied bone edit.":"Undid bone edit.");return true;
    }
    if(change.rig_revision != active->project->rig_edit_revision) {
        status("Undo the bone edits in Model before restoring this edit.");
        return false;
    }
    NativeFighterAuthoring* names = nullptr;
    if(!change.before_fighter.empty() && (change.before_fighter.size()!=change.after_fighter.size()||active->project->fighter_size<change.before_fighter.size()))return false;
    if (change.labels_saved) {
        const auto& bytes = redo ? change.after_labels : change.before_labels;
        if (!bytes.empty()) {
            names = (NativeFighterAuthoring*) std::malloc(bytes.size());
            if (!names) {
                return false;
            }
            std::memcpy(names, bytes.data(), bytes.size());
        }
    }
    if (!change.before_asset.empty()) {
        const std::vector<uint8_t>& asset =
            redo ? change.after_asset : change.before_asset;
        uint8_t* copy = (uint8_t*) std::malloc(asset.size());
        if (!copy) {
            std::free(names);
            return false;
        }
        std::memcpy(copy, asset.data(), asset.size());
        std::free(active->project->costume_dat);
        active->project->costume_dat = copy;
        active->project->costume_size = asset.size();
    } else {
        std::memcpy(active->project->costume_dat + change.offset,
                    redo ? change.after : change.before, change.count);
    }
    if (change.labels_saved) {
        std::free(active->project->authoring);
        active->project->authoring = names;
        nativeFighterAuthoring_Save(active->path, active->project);
    }
    // Restore only reference bytes changed by this model operation. Preserve
    // unrelated action, attribute and clip edits made in the other workspaces.
    const auto& fighter=redo?change.after_fighter:change.before_fighter;
    for(size_t i=0;i<change.before_fighter.size();++i)
        if(change.before_fighter[i]!=change.after_fighter[i])active->project->fighter_dat[i]=fighter[i];
    if (redo) ++active->change_cursor;
    else --active->change_cursor;
    buttons();
    reopen_model();
    if (!change.before_asset.empty())
        nativeShellWorkshop_RefreshPreview();
    nativeHome_WorkshopPoseInvalidate();
    status(redo ? "Reapplied Model edit." : "Undid Model edit.");
    return true;
}
