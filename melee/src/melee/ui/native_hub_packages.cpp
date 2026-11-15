#include "native_hub_packages.h"
#include "native_shell_workshop.h"
#include <melee/ft/native_costume_catalog.h>
#include <melee/ft/native_fighter_family_project.h>
#include <melee/gm/native_home.h>
#include <melee/gm/native_online_session.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
    struct Package { NativeContentId id; char title[NATIVE_CONTENT_TITLE_BYTES]; NativeNetplayGiftKind kind; } packages[512];
    NativeFighterDraft drafts[128];
    size_t count, draft_count;
    unsigned first, recipient_first;
    int selected = -1;
    bool draft_tab, installed_seen;
    NativeHubPage return_page = NativeHubPage_Game;
    char status[160], recipient[65];
    constexpr uint32_t gold = 0xE5C361FF, muted = 0xB9ADBDFF;
    void message(const char* text) { std::snprintf(status,sizeof(status),"%s",text); }
    void refresh()
    {
        count = draft_count = 0; selected = -1;
        char path[512]; const char* root = nativeHubCatalog_Root(path);
        NativeContentId ids[512]; size_t total=0;
        if (!root || !nativeContentStore_ListInstalled(root,ids,512,&total)) message("Could not read the complete installed collection.");
        else for (size_t i=0;i<total;++i) {
            uint8_t* bytes=nullptr; size_t size=0;
            if (!nativeContentStore_Read(root,&ids[i],&bytes,&size)) continue;
            Package entry = {}; entry.id = ids[i];
            NativeCostumePackage outfit; NativeCostumePackageData outfit_data;
            NativeFighterPackage fighter; NativeFighterPackageData fighter_data;
            if (nativeContentStore_ParseCostume(bytes,size,&outfit,&outfit_data)) {
                entry.kind=NativeNetplayGift_Costume; std::snprintf(entry.title,sizeof(entry.title),"%s",outfit.title);
            } else if (nativeContentStore_ParseFighter(bytes,size,&fighter,&fighter_data)) {
                entry.kind=NativeNetplayGift_Fighter; std::snprintf(entry.title,sizeof(entry.title),"%s",fighter.title);
            }
            std::free(bytes); if (entry.kind) packages[count++]=entry;
        }
        std::sort(packages,packages+count,[](const Package& a,const Package& b) {
            int title=std::strcmp(a.title,b.title); return title ? title<0 : std::memcmp(&a.id,&b.id,sizeof(a.id))<0;
        });
        if (!nativeFighterFamilyProject_ListLocal(drafts,128,&draft_count)) message("Could not list every fighter draft.");
        size_t size=draft_tab?draft_count:count; first=size?std::min(first,unsigned((size-1)/4*4)):0;
    }
    const NativeSignalingParticipant* target()
    {
        const auto* presence=nativeOnlineSession_Presence();
        if (presence) for (unsigned i=0;i<presence->participant_count;++i) {
            const auto& p=presence->participants[i];
            if (!std::strcmp(p.peer_id,recipient) && std::strcmp(p.peer_id,nativeOnlineSession_LocalPeerId())) return &p;
        }
        return nullptr;
    }
    void transfer()
    {
        NativeNetplayGiftStatus gift;
        if (!nativeOnlineSession_GiftStatus(false,&gift)) return;
        char text[192]; text[0]=0;
        switch (gift.state) {
        case NativeNetplayGift_Offered: std::snprintf(text,sizeof(text),"Waiting for acceptance: %.64s",gift.title); break;
        case NativeNetplayGift_Sending: std::snprintf(text,sizeof(text),"Sending %.64s: %u%%",gift.title,gift.bytes_total?unsigned(uint64_t(gift.bytes_done)*100/gift.bytes_total):0); break;
        case NativeNetplayGift_Sent: std::snprintf(text,sizeof(text),"Sent: %.64s",gift.title); break;
        case NativeNetplayGift_AwaitingReceipt: std::snprintf(text,sizeof(text),"Waiting for installation: %.64s",gift.title); break;
        case NativeNetplayGift_Declined: std::snprintf(text,sizeof(text),"Declined: %.64s",gift.title); break;
        case NativeNetplayGift_Failed: std::snprintf(text,sizeof(text),"Could not send: %.64s",gift.title); break;
        default: break;
        }
        nativeMenuPaint_Text(text,40,468,555,10,muted);
    }
    void edit(const char* path)
    {
        nativeShellWorkshop_Open(path,NativeWorkshopOrigin::Collection);
        hub.returning_from_tool=nativeShellWorkshop_IsOpen(); hub.visible=!hub.returning_from_tool;
    }
}
bool nativeHubPackages_Page(NativeHubPage page)
{
    return page == NativeHubPage_Packages || page == NativeHubPage_PackageRecipient;
}
void nativeHubPackages_Open()
{
    if (!nativeHome_IsSelection() && !nativeHome_IsPractice()) { nativeHub_Notice("Return to character select to manage packages."); return; }
    if (!nativeHubPackages_Page(hub.page)) return_page=hub.page;
    status[0]=0; refresh(); nativeHub_Open(NativeHubPage_Packages);
}
void nativeHubPackages_Tick()
{
    NativeNetplayGiftStatus gift;
    bool installed=nativeOnlineSession_GiftStatus(true,&gift) && gift.state==NativeNetplayGift_Installed;
    if (installed && !installed_seen) {
        if (nativeHubPackages_Page(hub.page) && hub.visible) refresh();
        if (gift.kind==NativeNetplayGift_Costume) nativeCostumeCatalog_Refresh();
    }
    installed_seen=installed;
}
bool nativeHubPackages_Activate(HubAction action,int value)
{
    if (action==HubAction::Packages) { nativeHubPackages_Open(); return true; }
    if (!nativeHubPackages_Page(hub.page)) return false;
    switch (action) {
    case HubAction::Back:
        nativeHub_ChangePage(hub.page==NativeHubPage_PackageRecipient ? NativeHubPage_Packages : return_page,true); return true;
    case HubAction::PackageTab: draft_tab=value!=0; first=0; selected=-1; return true;
    case HubAction::PackagePage: first=unsigned(std::max(0,int(first)+value)); return true;
    case HubAction::PackageRefresh: status[0]=0; refresh(); return true;
    case HubAction::PackageEdit:
        if (value<0) edit(nullptr);
        else if (size_t(value)<draft_count) edit(drafts[value].path);
        return true;
    case HubAction::PackageSend:
        if (value>=0 && size_t(value)<count) {
            selected=value; recipient[0]=0; recipient_first=0; status[0]=0;
            nativeHub_ChangePage(NativeHubPage_PackageRecipient);
        }
        return true;
    case HubAction::PackageRecipient:
        if (const auto* presence=nativeOnlineSession_Presence()) {
            if (value>=0 && unsigned(value)<presence->participant_count && std::strcmp(presence->participants[value].peer_id,nativeOnlineSession_LocalPeerId()))
                std::snprintf(recipient,sizeof(recipient),"%s",presence->participants[value].peer_id);
        }
        return true;
    case HubAction::PackageRecipientPage: recipient_first=unsigned(std::max(0,int(recipient_first)+value)); return true;
    case HubAction::PackageConfirm:
        if (selected<0 || size_t(selected)>=count || !target()) message("Choose someone who is still in the lobby.");
        else if (!nativeOnlineSession_OfferGift(recipient,&packages[selected].id)) message("Could not offer that package. Try again when the current transfer finishes.");
        else message("Package offered. They can choose whether to keep it.");
        return true;
    default: return false;
    }
}
void nativeHubPackages_Render(NativeHubPage page)
{
    nativeHub_Button("collection-close","< Back",40,402,110,28,HubAction::Back);
    nativeMenuPaint_Text(status,40,450,555,11,muted); transfer();
    if (page==NativeHubPage_PackageRecipient) {
        nativeMenuPaint_Text(selected>=0 && size_t(selected)<count ? packages[selected].title : "Choose a package",47,115,530,18,gold,NativeMenuFont_MenuOption);
        const auto* presence=nativeOnlineSession_Presence(); unsigned total=0,row=0;
        if (presence) for (unsigned i=0;i<presence->participant_count;++i) {
            const auto& p=presence->participants[i]; if (!std::strcmp(p.peer_id,nativeOnlineSession_LocalPeerId())) continue;
            if (total++<recipient_first || row>=6) continue;
            char id[40]; std::snprintf(id,sizeof(id),"collection-recipient-%u",i);
            nativeHub_Button(id,*p.name?p.name:"Player",50,157+row*34,530,28,HubAction::PackageRecipient,int(i),!std::strcmp(recipient,p.peer_id)); ++row;
        }
        if (!total) nativeHub_TextBlock("Join a lobby with another player to send a fighter or outfit.",50,187,530,80,16,muted);
        nativeHub_Button("collection-recipients-prev","<",50,367,42,25,HubAction::PackageRecipientPage,-6,false,recipient_first>0);
        nativeHub_Button("collection-recipients-next",">",103,367,42,25,HubAction::PackageRecipientPage,6,false,recipient_first+6<total);
        nativeHub_Button("collection-confirm","Send package",385,402,195,28,HubAction::PackageConfirm,0,true,target()!=nullptr && selected>=0);
        return;
    }
    nativeHub_Button("collection-installed","Installed packages",45,105,255,30,HubAction::PackageTab,0,!draft_tab);
    nativeHub_Button("collection-drafts","Fighter drafts",320,105,274,30,HubAction::PackageTab,1,draft_tab);
    size_t total=draft_tab?draft_count:count;
    for (unsigned row=0;row<4 && first+row<total;++row) {
        unsigned i=first+row; float y=152+row*53;
        char id[40],detail[96];
        nativeMenuPaint_Text(draft_tab?drafts[i].title:packages[i].title,53,y,398,15,gold,NativeMenuFont_MenuOption);
        if (draft_tab) std::snprintf(detail,sizeof(detail),"Fighter draft");
        else {
            char hex[65]; nativeContentStore_IdHex(&packages[i].id,hex);
            std::snprintf(detail,sizeof(detail),"%s  %.12s",packages[i].kind==NativeNetplayGift_Fighter?"Fighter":"Outfit",hex);
        }
        nativeMenuPaint_Text(detail,53,y+20,398,10,muted);
        std::snprintf(id,sizeof(id),"collection-%s-%u",draft_tab?"draft":"send",i);
        nativeHub_Button(id,draft_tab?"Edit":"Send",472,y,112,28,draft_tab?HubAction::PackageEdit:HubAction::PackageSend,int(i));
    }
    if (!total) nativeMenuPaint_Text(draft_tab?"No fighter drafts.":"No installed packages.",53,183,520,16,muted);
    nativeHub_Button("collection-prev","<",50,367,42,25,HubAction::PackagePage,-4,false,first>0);
    nativeHub_Button("collection-next",">",103,367,42,25,HubAction::PackagePage,4,false,first+4<total);
    char label[48]; std::snprintf(label,sizeof(label),"%zu %s",total,draft_tab?"drafts":"installed");
    nativeMenuPaint_Text(label,162,376,230,12,muted);
    nativeHub_Button("collection-refresh","Refresh",472,367,112,25,HubAction::PackageRefresh);
    nativeHub_Button("collection-workshop","Open Workshop",385,402,199,28,HubAction::PackageEdit,-1);
}
void nativeHubPackages_Inspect()
{
    std::fprintf(stderr,"[hub-inspect] packages page=%u count=%zu drafts=%zu first=%u tab=%u selected=%d recipient=%s status=%s\n",
        unsigned(hub.page),count,draft_count,first,draft_tab,selected,recipient,status);
    NativeNetplayGiftStatus gift;
    if (nativeOnlineSession_GiftStatus(false,&gift))
        std::fprintf(stderr,"[hub-inspect] package-transfer state=%u kind=%u peer=%s title=%s bytes=%u/%u\n",
            unsigned(gift.state),unsigned(gift.kind),gift.peer_id,gift.title,gift.bytes_done,gift.bytes_total);
    for (size_t i=first;i<count && i<first+4;++i) {
        char hex[65]; nativeContentStore_IdHex(&packages[i].id,hex);
        std::fprintf(stderr,"[hub-inspect] package index=%zu kind=%u title=%s id=%s\n",i,unsigned(packages[i].kind),packages[i].title,hex);
    }
}
