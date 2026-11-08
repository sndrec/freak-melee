#include "native_hub_startup_roster.h"
#include "native_hub_startup.h"
#include "native_menu_portrait_capture.h"
#include <melee/mn/native_menu_fighter.h>
#include <melee/mn/native_menu_scene.h>
#include <SDL3/SDL_timer.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
    // Fresh costume-zero visibility from the stock fighters' OnDeath setup.
    // Set bits start at variant zero; the remaining groups start hidden.
    struct Starter { FighterKind kind; const char* description; unsigned visible_groups = 1; };
    constexpr Starter starters[] = {
        {Ft_Kind_Mario, "Fireballs, a reflecting cape and an uppercut."},
        {Ft_Kind_DrMario, "Pills, a reflecting cape and strong close-range attacks."},
        {Ft_Kind_Luigi, "Slippery movement, fireballs and explosive launches."},
        {Ft_Kind_Koopa, "A heavy fighter with powerful claws and fire breath."},
        {Ft_Kind_Peach, "Float through the air, pull turnips and fight with Toad.", 0x57},
        {Ft_Kind_Yoshi, "Eggs, a strong double jump and a rolling egg form."},
        {Ft_Kind_Donkey, "Heavy punches, long reach and the ability to carry opponents."},
        {Ft_Kind_Captain, "Fast movement, explosive kicks and the Falcon Punch."},
        {Ft_Kind_Ganon, "A heavy fighter with powerful punches and kicks."},
        {Ft_Kind_Falco, "Lasers, a reflector and a high jump."},
        {Ft_Kind_Fox, "Fast movement, rapid lasers and a reflector."},
        {Ft_Kind_Ness, "Psychic attacks, a bat and a guided ball of lightning."},
        {Ft_Kind_Popo, "Popo and Nana fight together with hammers and ice.", 3},
        {Ft_Kind_Samus, "Charge shots, missiles, bombs and a long grab."},
        {Ft_Kind_Zelda, "Magic attacks and the ability to transform into Sheik.", 3},
        {Ft_Kind_Link, "A sword, shield, bow, bombs and a returning boomerang.", 7},
        {Ft_Kind_CLink, "A quick swordsman with fire arrows, bombs and a boomerang.", 7},
        {Ft_Kind_Pichu, "A small electric fighter whose lightning causes self-damage."},
        {Ft_Kind_Pikachu, "Electric attacks, thunder and a quick recovery.", 3},
        {Ft_Kind_Purin, "Multiple jumps, aerial movement and a powerful Rest."},
        {Ft_Kind_Mewtwo, "Psychic projectiles, a long tail and teleportation."},
        {Ft_Kind_GameWatch, "A flat fighter with tools, food and a damage-absorbing bucket.", 0xD},
        {Ft_Kind_Mars, "A swordsman whose blade is strongest at its tip.", 3},
        {Ft_Kind_Emblem, "A swordsman whose blade is strongest near the hilt.", 3},
    };
    constexpr int count = sizeof(starters) / sizeof(starters[0]);
    struct Cached { int index = -1; bool loaded, companion; } cache[3];
    int current, direction;
    Uint64 selected_at;

    int wrap(int index) { return (index % count + count) % count; }
    int slot_for(int index)
    {
        for (int slot = 0; slot < 3; ++slot) if (cache[slot].index == index) return slot;
        return -1;
    }
    void load_selection(int index, int movement)
    {
        current = wrap(index); direction = movement; selected_at = SDL_GetTicks();
        hub.template_kind = starters[current].kind;
        std::snprintf(hub.title, sizeof(hub.title), "%s", nativeFighterProject_StockBaseName(starters[current].kind));
        const int wanted[] = {wrap(current - 1), current, wrap(current + 1)};
        for (int target : wanted) {
            if (slot_for(target) >= 0) continue;
            int slot = 0;
            while (slot < 3 && (cache[slot].index == wanted[0] || cache[slot].index == wanted[1] || cache[slot].index == wanted[2])) ++slot;
            if (slot == 3) continue;
            nativeMenuFighter_Unload(slot);
            nativeMenuFighter_Unload(slot + 3);
            cache[slot].index = target;
            short variants[11];
            for (unsigned group = 0; group < 11; ++group) variants[group] = starters[target].visible_groups & (1u << group) ? 0 : -1;
            cache[slot].loaded = nativeMenuFighter_Load(slot, starters[target].kind, 0, nullptr, variants);
            if (starters[target].kind == Ft_Kind_GameWatch && cache[slot].loaded)
                nativeMenuFighter_SetFlatStyle(slot, 0x000000FF, 0xFFFFFF80, .01f, 1.57079633f);
            cache[slot].companion = starters[target].kind == Ft_Kind_Popo && nativeMenuFighter_Load(slot + 3, Ft_Kind_Nana, 0, nullptr, variants);
        }
    }
}

void nativeHubStartupRoster_Begin()
{
    int selected = 0;
    for (int i = 0; i < count; ++i) if (starters[i].kind == hub.template_kind) selected = i;
    load_selection(selected, 0);
}

void nativeHubStartupRoster_Select(int kind)
{
    for (int i = 0; i < count; ++i) if (starters[i].kind == kind) {
        if (i != current) load_selection(i, i == wrap(current + 1) ? 1 : -1);
        return;
    }
}

void nativeHubStartupRoster_Move(int movement)
{
    if (movement == -1 || movement == 1) load_selection(current + movement, movement);
}

void nativeHubStartupRoster_Render()
{
    constexpr uint32_t gold = 0xE5C361FF;
    nativeMenuPaint_Text("Choose your first fighter", 242, 99, 350, 27, gold, NativeMenuFont_MenuCaption);
    Mtx canvas; Mtx44 projection;
    nativeMenuScene_View(false, false, canvas, projection);
    float t = std::clamp((SDL_GetTicks() - selected_at) / 300.f, 0.f, 1.f);
    float shift = direction * (1 - t) * (1 - t) * (1 - t);
    float frame = SDL_GetTicks() * .06f;
    const int order[] = {-1, 1, 0};
    for (int delta : order) {
        int index = wrap(current + delta), slot = slot_for(index);
        float position = delta + shift;
        float weight = std::max(0.f, 1 - std::fabs(position));
        float x = 320 + position * 178, height = 95 + weight * 60;
        bool pair = index >= 0 && starters[index].kind == Ft_Kind_Popo;
        if (slot >= 0 && cache[slot].loaded) {
            nativeMenuFighter_Draw(slot, canvas, x - (pair ? height * .23f : 0), 322,
                height * (pair ? .84f : 1), -.35f - position * .55f, frame);
            if (cache[slot].companion) nativeMenuFighter_Draw(slot + 3, canvas, x + height * .23f, 322,
                height * .84f, .35f - position * .55f, frame);
        }
        nativeMenuPaint_Begin(canvas);
        char id[40]; std::snprintf(id, sizeof(id), "setup-fighter-%d", starters[index].kind);
        nativeHub_Region(id, x - 62, 148, 124, 177, HubAction::StockTemplate, starters[index].kind);
    }
    nativeHub_Button("setup-fighter-prev", "<", 35, 229, 39, 41, HubAction::StartupFighterMove, -1);
    nativeHub_Button("setup-fighter-next", ">", 566, 229, 39, 41, HubAction::StartupFighterMove, 1);
    nativeMenuPaint_Text(nativeFighterProject_StockBaseName(starters[current].kind), 52, 334, 540, 35,
        gold, NativeMenuFont_MenuCaption);
    nativeMenuPaint_Text(starters[current].description, 63, 377, 514, 11, 0xEEE9EDFF);
    nativeHub_Button("create-clone", "Choose fighter", 412, 421, 190, 35, HubAction::StartupConfirmFighter, 0, true);
}

void nativeHubStartupRoster_Confirm()
{
    constexpr uint32_t gold = 0xE5C361FF;
    Mtx canvas; Mtx44 projection;
    nativeMenuScene_View(false, false, canvas, projection);
    int slot = slot_for(current);
    if (slot >= 0 && cache[slot].loaded) {
        bool pair = cache[slot].companion;
        nativeMenuFighter_Draw(slot, canvas, pair ? 274 : 320, 327, pair ? 136 : 162, -.35f, 0);
        if (pair) nativeMenuFighter_Draw(slot + 3, canvas, 366, 327, 136, .35f, 0);
    }
    nativeMenuPaint_Begin(canvas);
    nativeMenuPaint_Text("Start with", 292, 102, 220, 20, 0xB9ADBDFF, NativeMenuFont_MenuOption);
    nativeMenuPaint_Text(nativeFighterProject_StockBaseName(starters[current].kind), 60, 132, 520, 33,
        gold, NativeMenuFont_MenuCaption, true);
    nativeHub_Button("setup-confirm-fighter", "Choose portrait", 389, 402, 213, 39,
        HubAction::StartupNext, 0, true);
}

unsigned nativeHubStartupRoster_Subjects(NativePortraitSubject subjects[2])
{
    int slot = slot_for(current);
    if (slot < 0 || !cache[slot].loaded) return 0;
    subjects[0] = { unsigned(slot) };
    unsigned count = cache[slot].companion ? 2 : 1;
    if (count == 2) {
        subjects[1] = {unsigned(slot + 3)};
        subjects[0].camera.pan_x = -.37f; subjects[1].camera.pan_x = .37f;
        subjects[0].camera.zoom = subjects[1].camera.zoom = .65f;
    }
    return count;
}

void nativeHubStartupRoster_Unload()
{
    nativeMenuFighter_UnloadAll();
    for (auto& entry : cache) entry = Cached{};
}

void nativeHubStartupRoster_Inspect()
{
    std::fprintf(stderr, "[hub-inspect] startup-roster count=%d selected=%d kind=%u name=%s\n",
        count, current, starters[current].kind, nativeFighterProject_StockBaseName(starters[current].kind));
    nativeMenuFighter_Inspect();
}

void nativeHubStartupRoster_Capture()
{
    char root[512], path[560];
    const char* folder = nativeHubCatalog_Root(root);
    int slot = slot_for(current);
    bool ok = nativeHubStartup.active && nativeHubStartup.step == NativeStartup_Clone && folder && slot >= 0;
    if (ok) {
        std::snprintf(path, sizeof(path), "%s/starter-portrait.png", folder);
        NativePortraitSubject subjects[2];
        unsigned count = nativeHubStartupRoster_Subjects(subjects);
        ok = nativeMenuPortraitCapture_Request(subjects, count, path);
    }
    std::fprintf(stderr, "[hub-inspect] portrait-request accepted=%d\n", ok);
}
