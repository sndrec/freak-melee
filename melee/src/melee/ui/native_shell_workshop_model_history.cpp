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
        const bool redo = !std::strcmp(button.first, "workshop-redo");
        const size_t index = redo ? active->change_cursor : active->change_cursor ? active->change_cursor - 1 : 0;
        element(button.first)->SetAttribute("title", index < active->changes.size() && !active->changes[index].summary.empty()
            ? active->changes[index].summary : redo ? "Redo Model edit" : "Undo Model edit");
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

bool record_asset(const std::vector<uint8_t>& before,const std::vector<uint8_t>& fighter)
{
    const bool same_costume=before.empty() || (before.size() == active->project->costume_size &&
        std::memcmp(before.data(), active->project->costume_dat,
                    before.size()) == 0);
    const bool same_fighter=fighter.empty() || (fighter.size()==active->project->fighter_size &&
        std::memcmp(fighter.data(),active->project->fighter_dat,fighter.size())==0);
    if(same_costume && same_fighter) return false;
    Change change = {};
    change.rig_revision = active->project->rig_edit_revision;
    change.before_asset = before;
    if (!before.empty()) change.after_asset.assign(active->project->costume_dat,
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
    return true;
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
    auto after=label_snapshot();
    if(before==after) return;
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
    change.after_labels = std::move(after);
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
    if(change.resources) {
        std::string error;
        if(!NativeResourceAuthoring::restore(*active->project,*change.resources,redo,error)) {status(error);return false;}
        if(redo)++active->change_cursor;else --active->change_cursor;
        buttons();status(redo?"Reapplied resource edit. Refresh preview to test it.":"Undid resource edit. Refresh preview to test it.");return true;
    }
    if(change.core) {
        auto before=std::make_unique<NativeFighterModelEditor>(active->model);
        NativeFighterAuthoring* names=nullptr;
        if(change.core->labels_saved) {
            const auto& expected=redo?change.core->before_labels:change.core->after_labels;
            const auto& replacement=redo?change.core->after_labels:change.core->before_labels;
            const auto* current=active->project->authoring;
            if((expected.empty() && current) || (!expected.empty() &&
                (!current || expected.size()!=sizeof(*current) || std::memcmp(expected.data(),current,expected.size())))) {
                status("Model labels or import metadata changed. Undo those edits first.");return false;
            }
            if(!replacement.empty()) {
                names=(NativeFighterAuthoring*)std::malloc(replacement.size());
                if(!names) {status("Could not reserve model metadata for Undo.");return false;}
                std::memcpy(names,replacement.data(),replacement.size());
            }
        }
        const uint64_t previous_rig=active->project->rig_edit_revision;
        char error[256];
        if(!nativeFighterCoreAssets_Restore(*active->project,
            redo?change.core->before:change.core->after,
            redo?change.core->after:change.core->before,error,sizeof(error))) {
            std::free(names);status(error);return false;
        }
        if(change.core->labels_saved) {std::free(active->project->authoring);active->project->authoring=names;}
        if(redo) ++active->change_cursor;else --active->change_cursor;
        playing=false;
        reopen_model(redo?&change.core->after_view:&change.core->before_view);
        if(previous_rig!=active->project->rig_edit_revision)
            nativeShellWorkshopEditor_RigChanged(*before,active->model);
        nativeShellWorkshop_RequestPreview(false);nativeHome_WorkshopPoseInvalidate();buttons();
        status(std::string(redo?"Redid: ":"Undid: ")+(change.summary.empty()?"Model edit":change.summary));return true;
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
    nativeShellWorkshop_RefreshAttributes();
    if (!change.before_asset.empty())
        nativeShellWorkshop_RequestPreview(false);
    nativeHome_WorkshopPoseInvalidate();
    status(redo ? "Reapplied Model edit." : "Undid Model edit.");
    return true;
}
