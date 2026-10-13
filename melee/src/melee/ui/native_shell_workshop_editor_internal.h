#pragma once

#include "native_shell_workshop_editor.h"

#include "native_shell_workshop.h"
#include "native_shell_workshop_bones.h"
#include "native_shell_workshop_hitboxes.h"
#include "native_shell_workshop_labels.h"
#include "native_shell_workshop_model.h"
#include "native_shell_workshop_parameters.h"
#include "native_shell_workshop_pose.h"
#include "native_shell_workshop_script_files.h"
#include "native_shell_workshop_transform.h"

#ifdef MELEE_NATIVE

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <melee/ft/native_fighter_action_editor.h>
#include <melee/ft/native_fighter_animation_editor.h>
#include <melee/ft/native_fighter_pose_editor.h>
#include <melee/ft/ftdrawcommon.h>
#include <melee/gm/native_home.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/native_fighter_behavior_source.h>

#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi/Core/Elements/ElementFormControlSelect.h>
#include <RmlUi/Core/Elements/ElementFormControlTextArea.h>

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <chrono>
#include <iterator>
#include <new>
#include <cmath>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace WorkshopEditor {

struct ActionEventTemplate {
    unsigned action;
    uint32_t offset;
    bool found;
};
struct TimelineEventInstance { unsigned event; int frame; };
struct GraphKeyRef { unsigned track; unsigned key; };
struct GraphKeyBaseline {
    GraphKeyRef ref;
    int frame;
    float value;
    float units_per_pixel;
    float screen_x;
    float screen_y;
};
struct EditedByte { uint32_t offset; uint8_t before; uint8_t after; };
struct EditHistoryEntry {
    bool animation;
    std::vector<EditedByte> bytes;
    std::vector<uint8_t> before_bank;
    std::vector<uint8_t> after_bank;
    std::vector<uint8_t> before_fighter;
    std::vector<uint8_t> after_fighter;
};
enum class GraphTool { Move, Box, Rotate, Scale };
enum class WorkshopMode { Action, Script, Animation, Model };
struct FighterEventEdit {
    uint8_t* before;
    size_t size;
    uint32_t offset;
    bool localized;
};
struct EditorWidgetState {
    std::string id, value;
    int selection_start = 0, selection_end = 0;
    float scroll_left = 0, scroll_top = 0;
    bool checked = false;
};
struct EditorElementState { std::string id, markup; };
struct EditorScrollState { std::string id; float left = 0, top = 0; };
struct EditorScriptView { int selection_start = 0, selection_end = 0; float left = 0, top = 0; };

struct EditorSession {
    NativeFighterProject* project;
    char project_path[512];
    std::string saved_script;
    size_t script_file_index;
    NativeFighterActionInfo actions[1024];
    bool action_has_clip[1024];
    float action_clip_frames[1024];
    NativeFighterActionEvent events[256];
    ActionEventTemplate event_templates[NativeFighterActionEvent_SetAirborneState + 1];
    bool event_templates_dirty = true;
    bool add_event_menu_open;
    NativeFighterActionCommand commands[512];
    NativeFighterAnimationTrack tracks[1024];
    NativeFighterAnimationKey keys[1024];
    bool selected_tracks[1024];
    bool skeleton_hierarchy_available;
    size_t action_count;
    size_t event_count;
    size_t command_count;
    size_t track_count;
    size_t key_count;
    int selected_action = -1;
    int selected_event = -1;
    int selected_command = -1;
    int selected_track = -1;
    int selected_key = -1;
    NativeFighterAnimationClip animation_clip = { NativeFighterAnimation_Action, UINT_MAX };
    NativeFighterAnimationClip displayed_animation_clip = { NativeFighterAnimation_Action, UINT_MAX };
    std::string animation_name;
    std::string demo_clip_names[128];
    float demo_clip_frames[128] = {};
    bool demo_has_clip[128] = {};
    size_t demo_count = 0;
    bool animation_dirty;
    bool bone_pose_refresh;
    bool action_dirty;
    bool script_loaded;
    bool normalize_values = true;
    bool graph_animated_only = true;
    bool show_animation_curves = true;
    bool graph_focused;
    float action_timeline_fraction = 0.32f;
    float animation_timeline_fraction = 0.43f;
    bool auto_key;
    bool show_event_list;
    bool hitbox_lanes_expanded = true;
    bool action_preview_playing;
    std::chrono::steady_clock::time_point action_preview_started;
    int action_preview_start_frame;
    bool show_hitboxes = true;
    bool show_hurtboxes;
    bool show_action_bones;
    bool show_animation_bones = true;
    bool action_group_open[5] = {};
    int timeline_frame;
    int event_frames[256];
    int command_frames[512];
    int timeline_end_frame;
    NativeFighterActionOccurrence timeline_occurrences[4096];
    size_t timeline_occurrence_count;
    TimelineEventInstance timeline_events[4096];
    size_t timeline_event_count;
    bool timeline_complete;
    NativeFighterHitbox preview_hitboxes[4];
    unsigned preview_hitbox_mask;
    int preview_hitbox_cached_frame = -1;
    int preview_hitbox_cached_action = -1;
    bool action_move_armed;
    int graph_width;
    int graph_height;
    float graph_units_per_pixel[1024];
    float graph_high_value[1024];
    bool graph_visible[1024];
    float graph_view_start;
    float graph_view_end;
    float clip_frames;
    unsigned animation_joint_count;
    int selected_outliner_joint = -1;
    int animation_frame;
    bool graph_refresh;
    bool graph_drag_active;
    unsigned drag_track;
    unsigned drag_key;
    int drag_start_x;
    int drag_start_y;
    std::vector<GraphKeyRef> graph_selection;
    std::vector<GraphKeyBaseline> graph_drag_baselines;
    GraphTool graph_tool = GraphTool::Move;
    bool graph_cursor_pivot;
    bool graph_box_active;
    int graph_box_start_x;
    int graph_box_start_y;
    std::vector<EditHistoryEntry> edit_history;
    size_t edit_cursor;
    NativeFighterPose copied_pose;
    bool pose_copied;
    int clip_scroll_delay;

    WorkshopMode mode = WorkshopMode::Action;


    WorkshopScriptFiles::Session script_files;
    std::vector<EditorWidgetState> widgets;
    std::vector<EditorElementState> messages;
    std::vector<EditorScrollState> scrolls;
    std::vector<EditorScriptView> script_views;
    std::string parameter_source;
    std::string focus_id;
};
extern Rml::ElementDocument* document;
extern EditorSession sessions[2];
extern EditorSession* session;
extern unsigned selected_member;
class GraphKeyEvents final : public Rml::EventListener {
public:
    void ProcessEvent(Rml::Event& event) override;
};
extern GraphKeyEvents graph_key_events;
class GraphTangentEvents final : public Rml::EventListener {
public:
    void Reset() { track = key = UINT_MAX; }
    void ProcessEvent(Rml::Event& event) override;
private:
    unsigned track = UINT_MAX;
    unsigned key = UINT_MAX;
    int side = 0;
    int mouse_x = 0;
    int mouse_y = 0;
    float handle_x = 0.0f;
    float handle_y = 0.0f;
};
extern GraphTangentEvents graph_tangent_events;
class GraphBoxEvents final : public Rml::EventListener {
public:
    void ProcessEvent(Rml::Event& event) override;
};
extern GraphBoxEvents graph_box_events;
class GraphNavigationEvents final : public Rml::EventListener {
public:
    void ProcessEvent(Rml::Event& event) override;
    void Reset() { dragging = false; }
private:
    bool dragging = false;
    bool zooming = false;
    int last_x = 0;
    int last_y = 0;
};
extern GraphNavigationEvents graph_navigation_events;
class ActionTimelineEvents final : public Rml::EventListener {
public:
    void Reset() { source_event = -1; }
    void ProcessEvent(Rml::Event& event) override;
private:
    int source_event = -1;
    int start_x = 0;
    int start_frame = 0;
};
extern ActionTimelineEvents action_timeline_events;
class TimelineRulerEvents final : public Rml::EventListener {
public:
    void Reset() { seeking = false; }
    void ProcessEvent(Rml::Event& event) override;
private:
    bool seeking = false;
};
extern TimelineRulerEvents timeline_ruler_events;
class WorkshopCameraEvents final : public Rml::EventListener {
public:
    void ProcessEvent(Rml::Event& event) override;
    void Reset() { dragging = false; }
private:
    bool dragging = false;
    int last_x = 0;
    int last_y = 0;
};
extern WorkshopCameraEvents workshop_camera_events;
class TimelineSplitterEvents final : public Rml::EventListener {
public:
    void Reset() { active = false; }
    void ProcessEvent(Rml::Event& event) override;
private:
    bool active = false;
    int start_y = 0;
    float start_fraction = 0.0f;
};
extern TimelineSplitterEvents timeline_splitter_events;


Rml::ElementFormControlInput* field(const char* id);
void set_text(const char* id, const std::string& value);
void update_history_buttons();
void record_edit(bool animation, const uint8_t* before);
void record_structural_edit(bool animation, const uint8_t* before, size_t size,
                            const uint8_t* before_fighter = nullptr,
                            size_t fighter_size = 0);
std::string number(unsigned value);
std::string float_text(float value);
std::string graph_tick_text(float value, float step);
bool parse_float(const char* id, float* result);
bool parse_row(const char* id, const char* prefix, unsigned* index);
bool parse_u16(const char* id, unsigned limit, uint16_t* value);
bool parse_u32(const char* id, uint32_t* value);
bool parse_i16(const char* id, int16_t* value);
void stop_action_preview();
void set_graph_focus(bool focused);
void toggle_action_preview();
void select_mode(WorkshopMode next);
void show_animation_keys();
void show_bone_transform();
void apply_bone_transform();
void key_bone_channels();
void record_pose_keys(const uint8_t* before, size_t size,
                      const uint8_t* before_fighter, size_t fighter_size);
void show_animation_graph();
void set_graph_tool(GraphTool tool);
void show_animation_dopesheet();
void select_key(unsigned key);
void show_track_browser();
void refresh_track_selection();
void select_track(unsigned track, bool toggle = false);
void select_joint(unsigned joint, bool toggle);
void show_animation();
void apply_clip_frames();
void apply_key();
void insert_key_at_frame();
void delete_selected_key();
void copy_pose_at_frame();
void paste_copied_pose(const char* destination);
std::string combined_script();
void show_script_files();
void select_script_file(size_t index);
void show_script();
bool validate_script();
void jump_to_script_line(unsigned line);
void show_frame_stack();
void show_action_playback();
void apply_action_playback();
void update_timeline_cursor();
int hitbox_end_for_instance(size_t instance);
int hitbox_end_frame(size_t index);
int timing_event_for_selected();
void show_event();
void show_commands();
void show_action_timeline();
void select_action(unsigned action);
void select_demo_clip(unsigned clip);
void read_demo_clips();
void select_event(unsigned index, bool seek_to_first_occurrence = true);
int active_hitbox_event(unsigned hitbox_id);
void refresh_event_templates();
void toggle_add_event_menu();
void insert_selected_event(unsigned template_action, uint32_t template_offset);
void delete_selected_event();
void apply_history(bool redo);
void select_command(unsigned index);
bool begin_fighter_event_edit(uint32_t offset, FighterEventEdit* edit);
void cancel_fighter_event_edit(FighterEventEdit* edit);
void commit_fighter_event_edit(FighterEventEdit* edit);
void apply_event_frame();
void apply_hitbox();
void apply_event_value();
void apply_sound();
void apply_effect();
void capture_session_widgets();
void restore_session_widgets();
void cancel_session_gestures();
void capture_script_view();
void restore_script_view();
} // namespace WorkshopEditor
#endif
