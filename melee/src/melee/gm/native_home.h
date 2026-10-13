#ifndef MELEE_GM_NATIVE_HOME_H
#define MELEE_GM_NATIVE_HOME_H

#include <stdbool.h>
#include <stdint.h>
#include <melee/ft/native_fighter_animation_editor.h>
#include <melee/gm/native_content_store.h>
#include <melee/mn/forward.h>

typedef struct NativeFighterHitbox NativeFighterHitbox;

#ifdef __cplusplus
extern "C" {
#endif

bool nativeHome_IsSelection(void);
bool nativeHome_IsPractice(void);
typedef struct NativeHomePracticeWatch {
    unsigned motion_id;
    float animation_frame;
    float velocity_x;
    float velocity_y;
    bool grounded;
} NativeHomePracticeWatch;
bool nativeHome_PracticeWatch(unsigned port, NativeHomePracticeWatch* result);
// Select an immutable package member for frozen authoring views/watch queries.
// Selection persists even when unavailable. An explicit dropdown may restart a
// dormant alternate's frozen preview through normal creation; internal refreshes
// pass false. Live Test Character always retains standard whole-family ordering.
bool nativeHome_WorkshopSelectMember(unsigned member, bool restart_preview);
// Read-only local creation policy. Only an owned alternate at the frozen preview
// port may start with member1; all other players/modes return ordinary member0.
unsigned nativeHome_WorkshopInitialMember(unsigned port);
// Authoring-only preview of a cached fighter's selected source action.
// Holds its fighter processes while the original renderer draws its pose.
bool nativeHome_WorkshopPose(NativeFighterAnimationClip clip, float frame);
void nativeHome_WorkshopSceneStarted(void);
bool nativeHome_WorkshopCameraActive(void);
void nativeHome_WorkshopCameraDrag(float dx, float dy, bool pan);
void nativeHome_WorkshopCameraZoom(float wheel_steps);
void nativeHome_WorkshopCameraReset(bool front);
// Populate the original collision-debug renderer from the paused action's
// active hitboxes. The caller supplies at most four slots, indexed by ID.
void nativeHome_WorkshopPreviewHitboxes(const NativeFighterHitbox* hitboxes,
                                        unsigned active_mask);
typedef struct NativeHomeWorkshopBoneScreen {
    float x;
    float y;
} NativeHomeWorkshopBoneScreen;
bool nativeHome_WorkshopBoneScreen(unsigned joint,
                                   NativeHomeWorkshopBoneScreen* result);
bool nativeHome_WorkshopWorldScreen(const float point[3],
                                    NativeHomeWorkshopBoneScreen* result);
bool nativeHome_WorkshopBoneParent(unsigned joint, unsigned* parent);
const char* nativeHome_WorkshopBoneRole(unsigned joint);
unsigned nativeHome_WorkshopBoneCount(void);
typedef struct NativeHomeWorkshopBoneLocal {
    float rotation[3];
    float scale[3];
    float position[3];
} NativeHomeWorkshopBoneLocal;
bool nativeHome_WorkshopBoneLocal(unsigned joint,
                                  NativeHomeWorkshopBoneLocal* result);
bool nativeHome_WorkshopSetBoneLocal(unsigned joint,
                                     const NativeHomeWorkshopBoneLocal* value);
// Transient editor visibility, hover tint and selected-bone heatmap.
typedef struct NativeHomeWorkshopMesh {
    unsigned joint, ordinal;
    bool hidden;
} NativeHomeWorkshopMesh;
void nativeHome_WorkshopMeshView(const NativeHomeWorkshopMesh* meshes,
                                 unsigned count, int highlight,
                                 int weight_mesh, unsigned weight_joint);
bool nativeHome_WorkshopBoneMatrix(unsigned joint, float result[3][4]);
bool nativeHome_WorkshopRay(float x, float y, float origin[3],
                            float direction[3]);
void nativeHome_WorkshopPoseInvalidate(void);
typedef struct NativeHomeWorkshopHitboxScreen {
    float x;
    float y;
    float direction_x;
    float direction_y;
    bool direction_valid;
} NativeHomeWorkshopHitboxScreen;
bool nativeHome_WorkshopHitboxScreen(unsigned hitbox, unsigned angle,
                                    NativeHomeWorkshopHitboxScreen* result);
void nativeHome_WorkshopPoseRelease(void);
void nativeHome_SetAuthoring(bool active);
bool nativeHome_PracticeTransitioning(void);
bool nativeHome_RequestAuthoringPractice(const NativeContentId* fighter, bool live_test);
bool nativeHome_RequestPreview(const NativeContentId* id, bool live_test);
bool nativeHome_ReturnToWorkshopPreview(void);
bool nativeHome_CanStartMatch(void);
bool nativeHome_TeamsNeedOpponents(void);
bool nativeHome_LocalRosterMatchesRoom(void);
bool nativeHome_RoomRosterReady(bool* owner_requested,
                                NativeContentId costume_packages[4],
                                NativeContentId fighter_packages[4]);
void nativeHome_RequestStartMatch(void);
void nativeHome_CancelStartMatch(void);
void nativeHome_EnterSelection(CSSData* css);
bool nativeHome_CssPlayer(unsigned port, uint8_t* character, uint8_t* color);
const NativeContentId* nativeHome_CostumePackage(unsigned port);
bool nativeHome_SelectCostumePackage(unsigned port, const NativeContentId* id);
const NativeContentId* nativeHome_FighterPackage(unsigned port);
void nativeHome_SelectFighterPackage(unsigned port, const NativeContentId* id);
void nativeHome_ReplaceInstalledFighter(const NativeContentId* previous,
                                       const NativeContentId* next);
bool nativeHome_LockSelection(CSSData* css);
int nativeHome_PhysicalPortForLocalPlayer(unsigned local_player);
void nativeHome_EnterPractice(StartMeleeData* start);
void nativeHome_ExitPractice(void);
void nativeHome_EnterResults(void);
void nativeHome_ApplyResultsInput(void);
void nativeHome_Tick(void);

#ifdef __cplusplus
}
#endif
#endif
