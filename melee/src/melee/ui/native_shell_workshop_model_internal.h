#pragma once
#include <melee/ft/native_fighter_gltf_import.h>
#include <melee/ft/native_fighter_model_editor.h>
#include <melee/ft/native_fighter_core_asset_state.h>
#include <melee/ft/native_fighter_action_editor.h>
#include <melee/gm/native_home.h>
#include <memory>
#include <climits>
#include <string>
#include <vector>
namespace Rml {class Element;class ElementDocument;class ElementFormControlInput;}
namespace WorkshopModelInternal {
enum class Context { Meshes, Materials, Skeleton, Skinning };
enum class RigMode { Edit, Pose, Animation };

struct RigView {
    unsigned count=0;
    int selected=0;
    bool initialized=false;
    uint32_t offsets[NATIVE_MODEL_MAX_JOINTS]={};
    bool selected_joints[NATIVE_MODEL_MAX_JOINTS]={};
    bool pose_changed[NATIVE_MODEL_MAX_JOINTS]={};
    NativeHomeWorkshopBoneLocal pose[NATIVE_MODEL_MAX_JOINTS]={};
};
struct RigChange {
    NativeFighterCoreAssetState before,after;
    RigView before_view,after_view;
};
struct Change {
    uint32_t offset;
    uint8_t count;
    uint8_t before[4];
    uint8_t after[4];
    std::vector<uint8_t> before_asset;
    std::vector<uint8_t> after_asset;
    bool labels_saved = false;
    std::vector<uint8_t> before_labels, after_labels;
    std::vector<uint8_t> before_fighter,after_fighter;
    std::unique_ptr<RigChange> rig;
    uint64_t rig_revision = 0;
};

struct ModelSession {
    NativeFighterProject* project = nullptr;
    char path[512] = {};
    bool model_available = false;
    NativeFighterModelEditor model = {};
    const NativeFighterActionInfo* actions = nullptr;
    const bool* has_clip = nullptr;
    const float* clip_frames = nullptr;
    size_t action_count = 0;
    Context context = Context::Skeleton;
    RigMode rig_mode = RigMode::Edit;
    int selected_mesh = -1;
    int selected_material = -1;
    int selected_joint = 0;
    bool selected_joints[NATIVE_MODEL_MAX_JOINTS] = {};
    int selected_clip = -1;
    int frame = 0;
    bool bones_visible = true;
    bool roles_applied = false;
    bool pose_initialized = false;
    uint8_t hidden_meshes[NATIVE_MODEL_MAX_MESHES] = {};
    NativeHomeWorkshopBoneLocal pose[NATIVE_MODEL_MAX_JOINTS] = {};
    bool pose_changed[NATIVE_MODEL_MAX_JOINTS] = {};
    std::vector<Change> changes;
    size_t change_cursor = 0;
    NativeFighterCoreAssetState saved_assets;
    std::vector<uint8_t> saved_authoring;
    char import_path[512] = {};
    bool reimport=false, reimport_whole=false, import_replace_material=false;
    uint32_t reimport_anchor=0;
    NativeGltfReimportPlan reimport_plan={};
    NativeGltfScan import_scan = {};
    bool import_selected[NATIVE_GLTF_MAX_PRIMITIVES] = {};
    unsigned import_step = 0;
    unsigned import_replacement = UINT_MAX;
    unsigned import_joint = 0;
    float import_scale = 1.0f;
    NativeGltfBindingMode import_binding = NATIVE_GLTF_BIND_AUTOMATIC;
    bool import_visible = false;
    float brush_radius = 5.0f;
    float brush_strength = 0.25f;
    bool brush_subtract = false;
};

extern Rml::ElementDocument* document;
extern ModelSession sessions[2];
extern ModelSession* active;
extern bool visible;
extern bool playing;
Rml::Element* element(const char*);
Rml::ElementFormControlInput* input(const char*);
void status(const std::string&);
std::string escape(const std::string&);
void buttons();
void record(uint32_t,unsigned,const uint8_t[4]);
bool record_asset(const std::vector<uint8_t>&,const std::vector<uint8_t>& fighter = {});
std::vector<uint8_t> label_snapshot();
void record_label_change(const std::vector<uint8_t>&,bool);
void reopen_model(const RigView* restored=nullptr);
void render();
void render_import();
bool import_click(const char*);
void text(const char*,const std::string&);
std::string decimal(float);
bool parse_float(const char*,float*);
bool index(const char*,const char*,unsigned*);
std::string rig_inspector();
std::string rig_tree_controls();
bool rig_click(const char*);
void rig_capture_view(RigView&);
void rig_restore_view(const RigView&);
}
