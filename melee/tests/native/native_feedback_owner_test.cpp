#include <melee/gm/native_feedback.h>
#include <melee/gm/native_feedback_context.h>
#include <melee/gm/native_feedback_input.h>
#include <melee/gm/native_community.h>
#include <melee/ui/native_feedback_view.h>
#include <melee/ui/native_hub.h>
#include <melee/ui/native_hub_catalog.h>
#include <melee/ui/native_shell_workshop.h>
#include <melee/ui/native_shell_workshop_editor.h>
#include <melee/mn/native_css_interaction.h>
#include <melee/mn/native_menu_scene.h>
#include <aurora/rmlui.hpp>
extern "C" {
#include <melee/lb/lb_0195.h>
#include <sysdolphin/baselib/controller.h>
}
#include <SDL3/SDL.h>
#include <yyjson.h>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace {
std::string root, status;
NativeFeedbackDraft* draft;
NativeCommunityUpload request;
NativeCommunityReceipt receipt;
NativeFeedbackAction action;
AuroraPresentedCapture picture;
bool accept_picture=true, accept_view=true, block_context, receipt_ready, shown;
unsigned collections, uploads, releases, checks;
std::string read(const char* path) {
    std::ifstream file(path,std::ios::binary);
    return {std::istreambuf_iterator<char>(file),{}};
}
bool partial(const std::string& bytes) {
    auto* doc=yyjson_read(bytes.data(),bytes.size(),0);
    if (!doc) return false;
    auto* object=yyjson_doc_get_root(doc);
    bool ok=yyjson_is_false(yyjson_obj_get(object,"context_complete")) &&
        !std::strcmp(yyjson_get_str(yyjson_obj_get(object,"id")),draft->id) &&
        !std::strcmp(yyjson_get_str(yyjson_obj_get(object,"captured_at")),draft->captured_at) &&
        yyjson_get_uint(yyjson_obj_get(object,"displayed_frame"))==42;
    yyjson_doc_free(doc);return ok;
}
}
#define CHECK(value) do { ++checks; if (!(value)) { std::fprintf(stderr,"line %u: %s\n",__LINE__,#value);return 1;} } while(0)

// Use the production report owner with controlled capture, view and service
// endpoints. No game, window, renderer or network connection is started.
namespace aurora::rmlui {
Rml::Context* get_context() noexcept { return nullptr; }
void handle_event(SDL_Event&) noexcept {}
}
bool aurora_capture_presented(const char* path,bool,AuroraPresentedCapture* output) {
    if (!accept_picture) return false;
    picture={};picture.capture_id=1;picture.displayed_frame=42;picture.presented_at_ns=100;
    picture.width=640;picture.height=480;picture.status=AURORA_CAPTURE_READY;*output=picture;
    FILE* file=std::fopen(path,"wb");if (file) {std::fputs("owned image bytes",file);std::fclose(file);}
    return true;
}
void aurora_presented_capture_status(AuroraPresentedCapture* output) { *output=picture; }
void aurora_release_presented_capture() { ++releases; }
void aurora_set_capture_backdrop(bool) {}
bool aurora_write_presented_capture(const char*) { return false; }
const char* nativeHubCatalog_Root(char*) { return root.c_str(); }
extern "C" bool nativeFeedbackContext_Write(const char* path,const char*,const char*,const AuroraPresentedCapture*,const NativeFeedbackInterface*) {
    ++collections;if (block_context) CreateDirectoryA(path,nullptr);return false;
}
extern "C" bool nativeFeedbackContext_Online(void) { return false; }
extern "C" void nativeFeedbackInput_SetOpen(bool) {}
extern "C" bool nativeHub_IsVisible(void) { return true; }
extern "C" NativeHubPage nativeHub_CurrentPage(void) { return NativeHubPage_Workshop; }
extern "C" void nativeHub_ReleasePointer(void) {}
extern "C" void nativeCssInteraction_CancelPointer(void) {}
extern "C" const char* nativeOnlineSession_RoomCode(void) { return ""; }
extern "C" bool nativeMenuScene_Active(void) { return false; }
extern "C" void HSD_PadFlushQueue(HSD_FlushType) {}
extern "C" void lbInput_RebaseClock(uint64_t) {}
bool nativeShellWorkshop_IsOpen() { return false; }
bool nativeShellWorkshop_IsTesting() { return false; }
bool nativeShellWorkshopEditor_IsModelMode() { return false; }
bool nativeShellWorkshopEditor_IsAnimationMode() { return false; }
bool nativeShellWorkshopEditor_IsScriptMode() { return false; }
bool nativeFeedbackView_Open(NativeFeedbackDraft& value) { draft=&value;shown=accept_view;return accept_view; }
void nativeFeedbackView_Close() { shown=false; }
void nativeFeedbackView_Update() {}
bool nativeFeedbackView_Key(int,bool) { return false; }
NativeFeedbackAction nativeFeedbackView_Action() { auto result=action;action=NativeFeedbackAction::None;return result; }
void nativeFeedbackView_Status(const char* message,bool,bool,bool) { status=message; }
void nativeFeedbackView_Confirm(bool) {}
bool nativeCommunity_Start(const char*,const char*) { return true; }
bool nativeCommunity_RequestUpload(const NativeCommunityUpload& value) { request=value;++uploads;return true; }
bool nativeCommunity_PollReceipt(NativeCommunityReceipt& value) {
    if (!receipt_ready)return false;
    receipt_ready=false;value=receipt;return true;
}

int main(int argc,char** argv)
{
    CHECK(argc==2);
    root=(std::filesystem::absolute(argv[1])/std::to_string(GetCurrentProcessId())).string();
    CHECK(!std::filesystem::exists(root));std::filesystem::create_directories(root);
    accept_picture=false;CHECK(!nativeFeedback_Open());CHECK(collections==0 && !shown);
    accept_picture=true;accept_view=false;
    CHECK(!nativeFeedback_Open());CHECK(releases==1 && !shown);
    CHECK(!std::filesystem::exists(draft->context_path) && !std::filesystem::exists(draft->image_path));
    accept_view=true;block_context=true;collections=0;
    CHECK(nativeFeedback_Open());CHECK(shown && nativeFeedback_IsOpen() && nativeFeedback_PausesSimulation());
    CHECK(collections==1 && status.find("still send")!=std::string::npos);
    action=NativeFeedbackAction::Send;nativeFeedback_Update();
    CHECK(uploads==0 && status.find("retained")!=std::string::npos);
    CHECK(RemoveDirectoryA(draft->context_path));block_context=false;
    action=NativeFeedbackAction::Send;nativeFeedback_Update();
    CHECK(uploads==1 && collections==1);
    const auto original=read(request.context), metadata=read(request.metadata);
    CHECK(partial(original));CHECK(metadata.find("\"context_complete\":false")!=std::string::npos);
    std::snprintf(receipt.id,sizeof(receipt.id),"%s",draft->id);
    receipt.revision=1;receipt_ready=true;std::snprintf(receipt.error,sizeof(receipt.error),"Retry this test upload.");
    nativeFeedback_Update();CHECK(status.find("Retry this test")!=std::string::npos);
    action=NativeFeedbackAction::Retry;nativeFeedback_Update();
    CHECK(uploads==2 && collections==1 && read(request.context)==original && read(request.metadata)==metadata);
    receipt.revision=2;receipt.stored=true;receipt_ready=true;nativeFeedback_Update();
    CHECK(!std::filesystem::exists(request.context) && !std::filesystem::exists(request.metadata));
    action=NativeFeedbackAction::Close;nativeFeedback_Update();CHECK(!shown && !nativeFeedback_IsOpen());
    CHECK(RemoveDirectoryA((root+"/feedback").c_str()));CHECK(RemoveDirectoryA(root.c_str()));
    std::printf("{\"status\":\"PASS\",\"checks\":%u}\n",checks);
}
