#include "native_shell_workshop_editor_internal.h"
#include <fstream>
#include <iostream>
using namespace WorkshopEditor;
static unsigned checks,gameplay_decode,shown,cancellations;
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<"line "<<__LINE__<<": "<<#x<<"\n";std::exit(1);}}while(0)
namespace WorkshopEditor {
Rml::ElementDocument* document;EditorSession sessions[2];EditorSession* session=&sessions[0];unsigned selected_member;
GraphKeyEvents graph_key_events;GraphTangentEvents graph_tangent_events;GraphBoxEvents graph_box_events;
GraphNavigationEvents graph_navigation_events;ActionTimelineEvents action_timeline_events;
TimelineRulerEvents timeline_ruler_events;WorkshopCameraEvents workshop_camera_events;TimelineSplitterEvents timeline_splitter_events;
void GraphKeyEvents::ProcessEvent(Rml::Event&){}void GraphTangentEvents::ProcessEvent(Rml::Event&){}
void GraphBoxEvents::ProcessEvent(Rml::Event&){}void GraphNavigationEvents::ProcessEvent(Rml::Event&){}
void ActionTimelineEvents::ProcessEvent(Rml::Event&){}void TimelineRulerEvents::ProcessEvent(Rml::Event&){}
void WorkshopCameraEvents::ProcessEvent(Rml::Event&){}void TimelineSplitterEvents::ProcessEvent(Rml::Event&){}
std::string number(unsigned n){return std::to_string(n);}
void stop_action_preview(){session->action_preview_playing=false;}
void set_graph_focus(bool b){session->graph_focused=b;}
void show_event(){}void show_action_playback(){}void show_commands(){}void show_action_timeline(){}
void update_timeline_cursor(){}void show_script(){}void show_script_files(){}
void show_animation(){++shown;}
void show_animation_graph(){++shown;}
void show_track_browser(){}void show_animation_keys(){}void select_key(unsigned n){session->selected_key=n;}
void select_track(unsigned n,bool){session->selected_track=n;}
std::string combined_script(){return {};}
}
// Decode/preview boundaries intentionally controlled; calls selecting gameplay are counted.
bool nativeFighterAnimation_Info(const NativeFighterProject* p,NativeFighterAnimationClip c,NativeFighterAnimationInfo* info){
 if(c.index>=2)return false;*info={};info->frames=c.bank==NativeFighterAnimation_Demo?50.0f:20.0f;
 const char* name=c.bank==NativeFighterAnimation_Demo?(c.index?"Pl_X_ACTION_Sleep_figatree":"Pl_X_ACTION_Win_figatree"):"Wait";
 info->name=name;return true;
}
bool nativeFighterAnimation_Keys(const NativeFighterProject*,NativeFighterAnimationClip clip,unsigned track,NativeFighterAnimationKey* keys,size_t capacity,size_t* count){
 if(clip.index!=0||track>=2)return false;
 const unsigned n=clip.bank==NativeFighterAnimation_Demo?3:2;if(capacity<n)return false;
 const int demo_frames[]={3,17,41},action_frames[]={5,8};
 for(unsigned i=0;i<n;++i)keys[i]={(clip.bank==NativeFighterAnimation_Demo?demo_frames[i]:action_frames[i]),2,(float)(clip.bank==NativeFighterAnimation_Demo?101+i:1+i),0,true,false};*count=n;return true;
}
bool nativeFighterAction_Commands(const NativeFighterProject*,unsigned,NativeFighterActionCommand*,size_t,size_t* n){++gameplay_decode;*n=0;return true;}
bool nativeFighterAction_Events(const NativeFighterProject*,unsigned,NativeFighterActionEvent*,size_t,size_t* n){++gameplay_decode;*n=0;return true;}
void ftDrawCommon_SetWorkshopOverlays(bool,bool){}
void nativeShellWorkshopModel_Show(bool){}bool nativeShellWorkshopModel_Select(unsigned){return true;}
bool nativeShellWorkshopModel_HasUnsavedEdits(unsigned){return false;}
void nativeShellWorkshopEditor_FilterActions(){}void nativeShellWorkshopEditor_RefreshScriptOutline(){}
void nativeWorkshopPose_Clear(){++cancellations;}void nativeWorkshopTransform_Clear(){}
void nativeHome_WorkshopPoseRelease(bool){}void nativeShellWorkshopBones_Reset(){}
void nativeShellWorkshopHitboxes_Update(bool,int,const NativeFighterHitbox*,unsigned){}
#include "actual.h"
class System final:public Rml::SystemInterface{bool LogMessage(Rml::Log::Type,const Rml::String& s)override{std::cerr<<s<<"\n";return true;}};
class Renderer final:public Rml::RenderInterface{
 Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex>,Rml::Span<const int>)override{return 1;}
 void RenderGeometry(Rml::CompiledGeometryHandle,Rml::Vector2f,Rml::TextureHandle)override{}void ReleaseGeometry(Rml::CompiledGeometryHandle)override{}
 Rml::TextureHandle LoadTexture(Rml::Vector2i&,const Rml::String&)override{return 0;}Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte>,Rml::Vector2i)override{return 1;}
 void ReleaseTexture(Rml::TextureHandle)override{}void EnableScissorRegion(bool)override{}void SetScissorRegion(Rml::Rectanglei)override{}
};
int main(int argc,char** argv){
 CHECK(argc==2);System sys;Renderer renderer;Rml::SetSystemInterface(&sys);Rml::SetRenderInterface(&renderer);CHECK(Rml::Initialise());CHECK(Rml::LoadFontFace("C:/Windows/Fonts/arial.ttf"));auto* context=Rml::CreateContext("clips",{800,600});CHECK(context);
 std::ifstream in(argv[1]);std::string text((std::istreambuf_iterator<char>(in)),{});document=context->LoadDocumentFromMemory(text);CHECK(document);document->Show();context->Update();
 // Controls belong beneath actual editor-owned roots, as in the shipped document.
 for(const char* id:{"workshop-action-search","workshop-animation-search"}){auto* e=document->GetElementById(id);auto node=e->GetParentNode()->RemoveChild(e);document->GetElementById("workshop-editor-browser")->AppendChild(std::move(node));}
 for(const char* id:{"workshop-script-source","workshop-parameters-source"}){auto* e=document->GetElementById(id);auto node=e->GetParentNode()->RemoveChild(e);document->GetElementById("workshop-script-pane")->AppendChild(std::move(node));}
 context->Update();
 static NativeFighterProject projects[2];
 for(unsigned m=0;m<2;++m){projects[m].demo_count=3;projects[m].fighter_size=4;projects[m].fighter_dat=(uint8_t*)std::calloc(1,4);projects[m].animation_size=4;projects[m].animation_dat=(uint8_t*)std::calloc(1,4);sessions[m].project=&projects[m];sessions[m].script_files.files.push_back({"common.fms","","state int value\n","state int value\n"});sessions[m].action_count=2;sessions[m].actions[0].has_script=true;std::strcpy(sessions[m].actions[0].name,"Wait");std::strcpy(sessions[m].actions[1].name,"AttackHi4");sessions[m].action_has_clip[0]=sessions[m].action_has_clip[1]=true;sessions[m].action_clip_frames[0]=sessions[m].action_clip_frames[1]=20;}
 select_action(0);CHECK(session->selected_action==0&&session->animation_clip.bank==NativeFighterAnimation_Action);unsigned gameplay=gameplay_decode;
 nativeShellWorkshopEditor_FilterClips();CHECK(document->GetElementById("workshop-clip-0")->IsClassSet("selected"));CHECK(!document->GetElementById("workshop-demo-0")->IsClassSet("selected"));
 select_demo_clip(0);CHECK(session->selected_action==0&&session->animation_clip.bank==NativeFighterAnimation_Demo&&session->animation_clip.index==0);CHECK(gameplay_decode==gameplay);CHECK(document->GetElementById("workshop-demo-0")->IsClassSet("selected"));CHECK(!document->GetElementById("workshop-clip-0")->IsClassSet("selected"));CHECK(!document->GetElementById("workshop-demo-2"));
 field("workshop-animation-search")->SetValue("sLeEp");nativeShellWorkshopEditor_FilterClips();CHECK(document->GetElementById("workshop-demo-1")&&!document->GetElementById("workshop-demo-0")&&!document->GetElementById("workshop-clip-0"));field("workshop-animation-search")->SetValue("");nativeShellWorkshopEditor_FilterClips();
 session->animation_frame=17;session->timeline_frame=4;select_mode(WorkshopMode::Animation);CHECK(session->animation_frame==17);select_mode(WorkshopMode::Action);CHECK(session->animation_frame==17&&session->timeline_frame==4);select_mode(WorkshopMode::Animation);CHECK(session->animation_frame==17&&session->animation_clip.bank==NativeFighterAnimation_Demo&&session->selected_action==0);CHECK(gameplay_decode==gameplay);
 // Same numeric index in a different bank is a new clip, not the remembered gameplay action.
 select_action(0);CHECK(session->animation_clip.bank==NativeFighterAnimation_Action&&session->animation_frame==0);select_demo_clip(0);session->animation_frame=17;
 CHECK(nativeShellWorkshopEditor_Select(1));select_action(0);select_demo_clip(1);session->animation_frame=29;select_mode(WorkshopMode::Animation);field("workshop-animation-search")->SetValue("Sleep");nativeShellWorkshopEditor_FilterClips();CHECK(nativeShellWorkshopEditor_Select(0));CHECK(field("workshop-animation-search")->GetValue().empty()&&document->GetElementById("workshop-demo-0"));CHECK(session->animation_clip.bank==NativeFighterAnimation_Demo&&session->animation_clip.index==0&&session->animation_frame==17);CHECK(nativeShellWorkshopEditor_Select(1));CHECK(session->animation_clip.index==1&&session->animation_frame==29);CHECK(field("workshop-animation-search")->GetValue()=="Sleep"&&document->GetElementById("workshop-demo-1")&&!document->GetElementById("workshop-demo-0"));CHECK(nativeShellWorkshopEditor_Select(0));
 // Actual history applies both banks, then reselects the same demo and restores its frame.
 session->project->animation_dat[0]=9;session->project->fighter_dat[0]=7;EditHistoryEntry edit={true,{}, {1,2,3,4},{9,8,7,6}};edit.before_fighter={3,0,0,0};edit.after_fighter={7,0,0,0};session->edit_history.push_back(edit);session->edit_cursor=1;
 apply_history(false);CHECK(session->edit_cursor==0&&session->project->animation_dat[0]==1&&session->project->fighter_dat[0]==3);CHECK(session->animation_clip.bank==NativeFighterAnimation_Demo&&session->animation_clip.index==0&&session->animation_frame==17&&session->selected_action==0);CHECK(document->GetElementById("workshop-demo-0")->IsClassSet("selected"));
 apply_history(true);CHECK(session->edit_cursor==1&&session->project->animation_dat[0]==9&&session->project->fighter_dat[0]==7);CHECK(session->animation_clip.bank==NativeFighterAnimation_Demo&&session->animation_frame==17);CHECK(sessions[1].edit_history.empty()&&sessions[1].animation_frame==29&&sessions[1].animation_clip.index==1);
 // Exact three corrected click branches, with deliberately different keys in Action0/Demo0.
 session->track_count=2;session->graph_visible[0]=true;session->graph_visible[1]=false;session->graph_view_start=4;session->graph_view_end=20;
 CHECK(fixture_clip_click("workshop-graph-key-0-1",false));CHECK(session->selected_track==0&&session->selected_key==1&&session->key_count==3&&session->keys[1].value==102&&session->keys[1].frame==17);
 CHECK(session->graph_selection.size()==1&&session->graph_selection[0].track==0&&session->graph_selection[0].key==1);
 CHECK(fixture_clip_click("workshop-graph-key-0-2",true));CHECK(session->graph_selection.size()==2&&session->selected_key==2&&session->keys[2].value==103);
 CHECK(fixture_clip_click("workshop-dope-key-0-0",false));CHECK(session->key_count==3&&session->keys[0].value==101&&session->selected_key==0&&session->graph_selection.size()==1);
 CHECK(fixture_clip_click("workshop-graph-select-all",false));CHECK(session->graph_selection.size()==1&&session->graph_selection[0].track==0&&session->graph_selection[0].key==1);
 CHECK(document->GetElementById("workshop-editor-status")->GetInnerRML()=="1 visible keys selected.");
 select_action(0);CHECK(fixture_clip_click("workshop-graph-key-0-1",false));CHECK(session->key_count==2&&session->keys[1].value==2&&session->keys[1].frame==8);
 CHECK(fixture_clip_click("workshop-dope-key-0-0",false));CHECK(session->key_count==2&&session->keys[0].value==1&&session->selected_key==0);
 CHECK(fixture_clip_click("workshop-graph-select-all",false));CHECK(session->graph_selection.size()==2&&session->graph_selection[0].key==0&&session->graph_selection[1].key==1);
 select_demo_clip(0);CHECK(fixture_clip_click("workshop-graph-select-all",false));CHECK(session->graph_selection.size()==1&&session->animation_clip.bank==NativeFighterAnimation_Demo&&session->selected_action==0);
 select_demo_clip(2);CHECK(session->animation_clip.index==0);CHECK(cancellations>=5&&shown>0);
 for(auto& p:projects){std::free(p.fighter_dat);std::free(p.animation_dat);}Rml::RemoveContext("clips");document=nullptr;Rml::Shutdown();std::cout<<"{\"checks\":"<<checks<<",\"status\":\"PASS\"}\n";
}
