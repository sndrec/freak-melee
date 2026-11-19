#include "native_match_launch.h"

#ifdef MELEE_NATIVE

#include <melee/ft/forward.h>
#include <melee/ft/native_costume_runtime.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/gr/forward.h>
#include <melee/pl/forward.h>

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gm_1601.h"
#include "gmmain_lib.h"
#include "gmvsmelee.h"
#include "native_match_selection.h"
#include "native_match_fixture.h"
#include "native_netplay_input.h"
#include "native_netplay_room.h"
#include "native_profile.h"
#include "native_profile_input.h"
#include "native_practice.h"
#include "native_settings.h"
#include "native_stock_content.h"
#include "types.h"
#include <melee/lb/lbdvd.h>
#include <melee/lb/types.h>
#include <melee/mn/types.h>

typedef struct NativeMatchSlot {
    u8 kind;
    u8 character;
    u8 cpu_level;
    u8 color;
    u8 team;
    u8 handicap;
    bool color_explicit;
    bool team_explicit;
} NativeMatchSlot;

typedef struct NativeMatchLaunch {
    bool direct;
    bool retail_menu;
    bool direct_prepared;
    bool direct_prepare_failed;
    bool time_explicit;
    bool replay_playback;
    bool replay_inputs_only;
    bool fast;
    bool hidden;
    bool muted;
    bool trace_actions;
    bool trace_fighter_state;
    bool trace_geometry;
    uint64_t trace_geometry_frame;
    bool local_preview;
    bool spectating;
    bool rollback_census;
    bool object_census;
    bool snapshot_census;
    bool rollback_probe;
    bool rollback_benchmark;
    bool rollback_correction_probe;
    bool rollback_correction_frame_explicit;
    unsigned rollback_correction_frame;
    bool teams;
    bool netplay_session_explicit;
    bool netplay_listen_explicit;
    bool netplay_local_peer_explicit;
    bool netplay_local_mask_explicit;
    bool netplay_delay_explicit;
    uint8_t artificial_delay_frames;
    bool input_delay_explicit;
    uint8_t input_delay_frames;
    bool netplay_room_explicit;
    bool netplay_room_id_explicit;
    bool netplay_peer_id_explicit;
    bool netplay_stun_explicit;
    bool costume_package_explicit[4];
    bool fighter_package_explicit[4];
    const char* replay_path;
    const char* content_root;
    const char* script_kind;
    const char* script_source;
    char profile_ids[4][NATIVE_PROFILE_ID_BYTES + 1];
    bool profile_explicit[4];
    NativeNetplayLaunchConfig netplay;
    u16 stage;
    u8 mode;
    u8 time_minutes;
    u8 stocks;
    s8 item_freq;
    NativeMatchSlot slots[4];
    NativeContentId costume_packages[4];
    NativeContentId fighter_packages[4];
    NativeContentId fighter_ids[4];
    uint8_t fighter_count;
    NativeLobbyMatch agreed_match;
    bool has_agreed_match;
} NativeMatchLaunch;

static NativeMatchLaunch launch;

static bool name_equals(const char* input, size_t length, const char* name)
{
    size_t i = 0;
    for (size_t at = 0; at < length; ++at) {
        unsigned char c = (unsigned char) input[at];
        if (c == '-' || c == '_' || c == ' ') {
            continue;
        }
        if (name[i] == 0 || tolower(c) != tolower((unsigned char) name[i])) {
            return false;
        }
        ++i;
    }
    return name[i] == 0;
}

static bool option_is(const char* input, const char* name)
{
    return name_equals(input, strlen(input), name);
}

static bool parse_number(const char* input, unsigned min, unsigned max,
                         unsigned* result)
{
    if (*input == 0 || !isdigit((unsigned char) *input)) {
        return false;
    }
    char* end;
    unsigned long parsed = strtoul(input, &end, 10);
    if (*end != 0 || parsed < min || parsed > max) {
        return false;
    }
    *result = (unsigned) parsed;
    return true;
}

static bool parse_u64(const char* input, uint64_t max, uint64_t* result)
{
    if (*input == 0) {
        return false;
    }
    char* end;
    errno = 0;
    unsigned long long parsed = strtoull(input, &end, 0);
    if (*end != 0 || errno == ERANGE || parsed > max) {
        return false;
    }
    *result = (uint64_t) parsed;
    return true;
}

static bool parse_netplay_peer(const char* input,
                               NativeNetplayPeerLaunch* result)
{
    char* end;
    unsigned long peer = strtoul(input, &end, 10);
    if (end == input || *end != ',' || peer > 3) {
        return false;
    }
    const char* address = end + 1;
    const char* address_end = strchr(address, ',');
    size_t address_length =
        address_end != NULL ? (size_t) (address_end - address) : 0;
    if (address_length == 0 || address_length >= sizeof(result->ipv4_address))
    {
        return false;
    }
    unsigned long port = strtoul(address_end + 1, &end, 10);
    if (end == address_end + 1 || *end != ',' || port == 0 ||
        port > UINT16_MAX)
    {
        return false;
    }
    uint64_t port_mask;
    if (!parse_u64(end + 1, 0xF, &port_mask) || port_mask == 0) {
        return false;
    }
    *result = (NativeNetplayPeerLaunch){
        .port = (uint16_t) port,
        .peer = (uint8_t) peer,
        .port_mask = (uint8_t) port_mask,
    };
    memcpy(result->ipv4_address, address, address_length);
    result->ipv4_address[address_length] = 0;
    return true;
}

static bool copy_option(char* output, size_t capacity, const char* input)
{
    size_t length = strlen(input);
    if (length == 0 || length >= capacity) {
        return false;
    }
    memcpy(output, input, length + 1);
    return true;
}

static bool parse_netplay_room_peer(const char* input,
                                    NativeNetplayRoomPeerLaunch* result)
{
    char* end;
    unsigned long peer = strtoul(input, &end, 10);
    if (end == input || *end != ',' || peer > 3) {
        return false;
    }
    const char* peer_id = end + 1;
    const char* peer_id_end = strchr(peer_id, ',');
    size_t peer_id_length =
        peer_id_end != NULL ? (size_t) (peer_id_end - peer_id) : 0;
    if (peer_id_length == 0 || peer_id_length >= sizeof(result->peer_id)) {
        return false;
    }
    uint64_t port_mask;
    if (!parse_u64(peer_id_end + 1, 0xF, &port_mask) || port_mask == 0) {
        return false;
    }
    *result = (NativeNetplayRoomPeerLaunch){
        .peer = (uint8_t) peer,
        .port_mask = (uint8_t) port_mask,
    };
    memcpy(result->peer_id, peer_id, peer_id_length);
    result->peer_id[peer_id_length] = '\0';
    return true;
}

static bool parse_host_port(const char* input, char* host,
                            size_t host_capacity, uint16_t* port)
{
    const char* separator = strrchr(input, ',');
    size_t host_length =
        separator != NULL ? (size_t) (separator - input) : 0;
    unsigned parsed_port;
    if (host_length == 0 || host_length >= host_capacity ||
        !parse_number(separator + 1, 1, UINT16_MAX, &parsed_port))
    {
        return false;
    }
    memcpy(host, input, host_length);
    host[host_length] = '\0';
    *port = (uint16_t) parsed_port;
    return true;
}

static bool parse_character(const char* input, size_t length, u8* result)
{
    if (name_equals(input, length, "custom")) {
        *result = ChKind_CustomFirst;
        return true;
    }
    static const struct {
        const char* name;
        u8 kind;
    } characters[] = {
        { "captainfalcon", CKind_Captain },
        { "donkeykong", CKind_Donkey },
        { "fox", CKind_Fox },
        { "mrgameandwatch", CKind_GameWatch },
        { "kirby", CKind_Kirby },
        { "bowser", CKind_Koopa },
        { "link", CKind_Link },
        { "luigi", CKind_Luigi },
        { "mario", CKind_Mario },
        { "marth", CKind_Mars },
        { "mewtwo", CKind_Mewtwo },
        { "ness", CKind_Ness },
        { "peach", CKind_Peach },
        { "pikachu", CKind_Pikachu },
        { "iceclimbers", CKind_PopoNana },
        { "jigglypuff", CKind_Purin },
        { "samus", CKind_Samus },
        { "yoshi", CKind_Yoshi },
        { "zelda", CKind_Zelda },
        { "sheik", CKind_Seak },
        { "falco", CKind_Falco },
        { "younglink", CKind_CLink },
        { "drmario", CKind_DrMario },
        { "roy", CKind_Emblem },
        { "pichu", CKind_Pichu },
        { "ganondorf", CKind_Ganon },
    };
    for (size_t i = 0; i < sizeof(characters) / sizeof(characters[0]); ++i) {
        if (name_equals(input, length, characters[i].name)) {
            *result = characters[i].kind;
            return true;
        }
    }
    return false;
}

static bool parse_slot(const char* input, NativeMatchSlot* slot)
{
    if (option_is(input, "off")) {
        slot->kind = Gm_PKind_NA;
        return true;
    }
    const char* colon = strchr(input, ':');
    if (colon == NULL) {
        return false;
    }
    if (name_equals(input, (size_t) (colon - input), "human")) {
        slot->kind = Gm_PKind_Human;
    } else if (name_equals(input, (size_t) (colon - input), "cpu")) {
        slot->kind = Gm_PKind_Cpu;
    } else {
        return false;
    }
    const char* character = colon + 1;
    colon = strchr(character, ':');
    size_t length = colon ? (size_t) (colon - character) : strlen(character);
    if (!parse_character(character, length, &slot->character)) {
        return false;
    }
    slot->cpu_level = 1;
    if (colon != NULL) {
        unsigned level;
        if (slot->kind != Gm_PKind_Cpu ||
            !parse_number(colon + 1, 1, 9, &level))
        {
            return false;
        }
        slot->cpu_level = (u8) level;
    }
    return true;
}

static bool parse_stage(const char* input, u16* result)
{
    if (option_is(input, "practice")) {
        *result = St_Kind_Practice;
    } else if (option_is(input, "battlefield")) {
        *result = St_Kind_Battle;
    } else if (option_is(input, "finaldestination") || option_is(input, "fd"))
    {
        *result = St_Kind_Last;
    } else if (option_is(input, "dreamland")) {
        *result = St_Kind_OldPupupu;
    } else if (option_is(input, "greengreens")) {
        *result = St_Kind_Greens;
    } else if (option_is(input, "junglejapes")) {
        *result = St_Kind_Garden;
    } else if (option_is(input, "yoshisstory")) {
        *result = St_Kind_Story;
    } else if (option_is(input, "flatzone")) {
        *result = St_Kind_Flatzone;
    } else if (option_is(input, "yoshisisland64")) {
        *result = St_Kind_OldYoshi;
    } else if (option_is(input, "kongojungle64")) {
        *result = St_Kind_OldKongo;
    } else if (option_is(input, "fountainofdreams")) {
        *result = St_Kind_Izumi;
    } else if (option_is(input, "rainbowcruise")) {
        *result = St_Kind_RCruise;
    } else if (option_is(input, "greatbay")) {
        *result = St_Kind_GreatBay;
    } else if (option_is(input, "hyruletemple")) {
        *result = St_Kind_Shrine;
    } else if (option_is(input, "onett")) {
        *result = St_Kind_Onett;
    } else if (option_is(input, "pokemonstadium")) {
        *result = St_Kind_PStadium;
    } else if (option_is(input, "brinstardepths")) {
        *result = St_Kind_Kraid;
    } else if (option_is(input, "fourside")) {
        *result = St_Kind_Fourside;
    } else if (option_is(input, "mushroomkingdom")) {
        *result = St_Kind_Inishie1;
    } else if (option_is(input, "mushroomkingdomii")) {
        *result = St_Kind_Inishie2;
    } else if (option_is(input, "pokefloats")) {
        *result = St_Kind_Pura;
    } else if (option_is(input, "bigblue")) {
        *result = St_Kind_BigBlue;
    } else if (option_is(input, "iciclemountain")) {
        *result = St_Kind_Icemt;
    } else if (option_is(input, "princesspeachscastle") ||
               option_is(input, "peachscastle"))
    {
        *result = St_Kind_Castle;
    } else if (option_is(input, "kongojungle")) {
        *result = St_Kind_Kongo;
    } else if (option_is(input, "corneria")) {
        *result = St_Kind_Corneria;
    } else if (option_is(input, "venom")) {
        *result = St_Kind_Venom;
    } else if (option_is(input, "brinstar")) {
        *result = St_Kind_Zebes;
    } else if (option_is(input, "mutecity")) {
        *result = St_Kind_MuteCity;
    } else if (option_is(input, "yoshisisland")) {
        *result = St_Kind_Yoster;
    } else {
        return false;
    }
    return true;
}

static bool parse_items(const char* input, s8* result)
{
    static const char* names[] = { "verylow", "low", "medium", "high",
                                   "veryhigh" };
    if (option_is(input, "off")) {
        *result = -1;
        return true;
    }
    for (int i = 0; i < 5; ++i) {
        if (option_is(input, names[i])) {
            *result = i;
            return true;
        }
    }
    return false;
}

static bool parse_team(const char* input, u8* result)
{
    static const char* names[] = { "red", "blue", "green" };
    for (u8 i = 0; i < 3; ++i) {
        if (option_is(input, names[i])) {
            *result = i;
            return true;
        }
    }
    return false;
}

static bool parse_player_option(const char* key, const char* suffix,
                                int* player)
{
    if (strlen(key) != strlen(suffix) + 4 || strncmp(key, "--p", 3) != 0 ||
        key[3] < '1' || key[3] > '4' || strcmp(key + 4, suffix) != 0)
    {
        return false;
    }
    *player = key[3] - '1';
    return true;
}

static u8 team_costume(u8 character, u8 team)
{
    switch (team) {
    case 0:
        return gm_80169264(character);
    case 1:
        return gm_801692BC(character);
    case 2:
        return gm_80169290(character);
    default:
        return 0;
    }
}

static bool resolve_fighter_packages(const NativeContentId packages[4],
                                     uint8_t kinds[4], bool require_installed,
                                     bool stage)
{
    NativeContentId ids[4] = { 0 };
    unsigned count = 0;
    for (unsigned port = 0; port < 4; ++port) {
        kinds[port] = ChKind_None;
        const NativeContentId empty = { 0 };
        if (memcmp(&packages[port], &empty, sizeof(empty)) == 0) {
            continue;
        }
        const NativeContentId* id = &packages[port];
        unsigned index = 0;
        while (index < count &&
               memcmp(ids[index].bytes, id->bytes,
                      NATIVE_CONTENT_ID_BYTES) < 0)
        {
            ++index;
        }
        if (index < count &&
            memcmp(ids[index].bytes, id->bytes,
                   NATIVE_CONTENT_ID_BYTES) == 0)
        {
            continue;
        }
        memmove(&ids[index + 1], &ids[index],
                (count - index) * sizeof(*id));
        ids[index] = *id;
        ++count;
    }
    char default_root[512];
    const char* root = launch.content_root;
    if (count != 0 && root == NULL) {
        if (!nativeContentStore_DefaultRoot(default_root,
                                            sizeof(default_root)))
        {
            return false;
        }
        root = default_root;
    }
    if (stage) {
        if (!nativeFighterRuntime_StageCached(ids, count, root,
                                              require_installed)) {
            fputs("Selected custom fighter could not be prepared.\n", stderr);
            return false;
        }
    } else {
        for (unsigned index = 0; index < count; ++index) {
            const NativeContentId* id = &ids[index];
            if ((require_installed &&
                 !nativeContentStore_IsInstalled(root, id)) ||
                !nativeFighterRuntime_ValidateCached(id, root))
            {
                fprintf(stderr,
                        "Custom fighter %u is missing or incompatible.\n",
                        index + 1);
                return false;
            }
        }
    }
    for (unsigned port = 0; port < 4; ++port) {
        for (unsigned index = 0; index < count; ++index) {
            if (memcmp(packages[port].bytes, ids[index].bytes,
                       NATIVE_CONTENT_ID_BYTES) == 0)
            {
                kinds[port] = ChKind_CustomFirst + index;
                break;
            }
        }
    }
    if (packages != launch.fighter_packages) {
        memcpy(launch.fighter_packages, packages,
               sizeof(launch.fighter_packages));
    }
    memcpy(launch.fighter_ids, ids, sizeof(ids));
    launch.fighter_count = count;
    return true;
}

bool nativeMatchLaunch_ConfigureLocalFighters(
    const NativeContentId packages[4], uint8_t kinds[4], bool preview)
{
    if (packages == NULL || kinds == NULL || launch.direct ||
        launch.netplay.enabled) {
        return false;
    }
    if (!resolve_fighter_packages(packages, kinds, !preview, true)) {
        return false;
    }
    launch.local_preview = preview;
    return true;
}

static bool resolve_direct_fighters(void)
{
    for (unsigned port = 0; port < 4; ++port) {
        bool custom = launch.slots[port].kind != Gm_PKind_NA &&
                      launch.slots[port].character == ChKind_CustomFirst;
        if (custom != launch.fighter_package_explicit[port]) {
            fprintf(stderr,
                    "P%u custom fighter needs its own --p%u-fighter ID.\n",
                    port + 1, port + 1);
            return false;
        }
    }
    uint8_t kinds[4];
    if (!resolve_fighter_packages(launch.fighter_packages, kinds, true,
                                  false)) {
        return false;
    }
    for (unsigned port = 0; port < 4; ++port) {
        if (launch.fighter_package_explicit[port]) {
            launch.slots[port].character = kinds[port];
        }
    }
    return true;
}

static void usage(const char* program)
{
    fprintf(
        stderr,
        "Usage: %s --iso <Melee USA v1.02.iso> [--match options]\n"
        "  --match                 Start a Versus match directly\n"
        "  --retail-menu           Start the original main menu for UI reference\n"
        "  --stage <name>          battlefield, final-destination, "
        "dream-land, green-greens, jungle-japes, yoshis-story, yoshis-island, "
        "flat-zone, yoshis-island-64, kongo-jungle-64, fountain-of-dreams, "
        "rainbow-cruise, great-bay, hyrule-temple, onett, pokemon-stadium, "
        "brinstar, brinstar-depths, fourside, mushroom-kingdom, "
        "mushroom-kingdom-ii, poke-floats, big-blue, icicle-mountain, "
        "princess-peachs-castle, kongo-jungle, corneria, mute-city, venom, "
        "practice (local)\n"
        "  --p1..--p4 <slot>       human:falco, cpu:luigi:3, human:custom, or off\n"
        "  --p1-profile..--p4-profile <id>  Local controller profile\n"
        "  --p1-color..--p4-color  FFA costume index, beginning at 0\n"
        "  --p1-skin..--p4-skin <id>  Installed costume package ID\n"
        "  --p1-fighter..--p4-fighter <id>  Installed custom fighter bundle ID\n"
        "  --content-root <path>  Alternate local content store for testing\n"
        "  --teams                 Enable team battle\n"
        "  --p1-team..--p4-team    red, blue, or green\n"
        "  --mode <kind>           time, stock, or coin\n"
        "  --time <minutes>        0..99 (0 disables stock timer)\n"
        "  --stocks <count>        1..99\n"
        "  --items <frequency>     off, very-low, low, medium, high, "
        "very-high\n"
        "  --record <file>         Record seed and all controller samples\n"
        "  --replay <file>         Replay and verify the same match options\n"
        "  --replay-inputs <file>  Compare recorded inputs with other content\n"
        "  --fixture-item <kind,sample,x,y>  Spawn a common item in an offline input comparison\n"
        "  --fixture-capturable-item <kind,sample,x,y>  Offline ordinary-item proxy with capture eligibility\n"
        "  --fixture-airborne <port,sample,x,y>  Begin a clean fall with jumps used in an offline comparison\n"
        "  --hidden                Keep rendering without displaying a window\n"
        "  --mute                  Silence host output while retaining audio simulation\n"
        "  --fast                  Run a direct match without real-time input "
        "pacing\n"
        "  --trace-actions         Log fighter motion and damage changes with "
        "replay sample numbers\n"
        "  --trace-fighter-state   Log fighter state/geometry and item state "
        "each replay sample\n"
        "  --trace-geometry-frame <sample>  Dump actual joints, materials and hitboxes at one replay sample\n"
        "  --rollback-census       Measure game-heap writes by simulation and "
        "rendering\n"
        "  --object-census         Untimed scene/tick object and allocator census\n"
        "  --snapshot-census       Offline partial-owner save/validation sizes and costs\n"
        "  --script-kind <number>  Offline interpreted fighter kind\n"
        "  --script-source <fms>   Validate and load source before actor creation\n"
        "  --rollback-probe        Restore and replay eight frames from "
        "--replay or --replay-inputs\n"
        "  --rollback-benchmark    64 verified 15-tick rollback windows after 600 ticks\n"
        "  --rollback-correction-probe\n"
        "  --rollback-correction-frame <240..591>  Target frame for the "
        "late-input probe\n"
        "                          Inject late input and replay its corrected "
        "timeline twice\n"
        "  --netplay-session <id>  Shared 64-bit session ID (decimal or 0x "
        "hex)\n"
        "  --netplay-listen <port> Local UDP port\n"
        "  --netplay-local-peer <0..3>\n"
        "  --netplay-local-mask <mask>  Locally owned P1..P4 bits\n"
        "  --netplay-peer <id,IPv4,port,mask>  Repeat for each remote PC\n"
        "  --netplay-room <origin>  Use room signaling and ICE instead of "
        "raw UDP\n"
        "  --netplay-room-id <id>  Private room identifier\n"
        "  --netplay-peer-id <id>  Stable local room identity\n"
        "  --netplay-room-peer <id,peer-id,mask>  Expected remote PC\n"
        "  --netplay-stun <host,port>  Optional ICE STUN server\n"
        "  --netplay-input-delay <frames>  Local input buffer, 0..4\n"
        "  --netplay-delay <frames>  Diagnostic outbound delay, 0..14 "
        "(direct or room)\n"
        "Defaults: Battlefield, Falco human P1, Luigi CPU level 1 P2,\n"
        "          2-minute time match, 4 stocks, items off.\n",
        program);
}

int nativeMatchLaunch_Parse(int argc, char** argv, const char** iso_path)
{
    nativeMatchFixture_Reset();
    nativeCostumeRuntime_Clear();
    launch = (NativeMatchLaunch) {
        .stage = St_Kind_Battle,
        .mode = MatchKind_Time,
        .time_minutes = 2,
        .stocks = 4,
        .item_freq = -1,
        .netplay.room = {
            .build = "freak-melee-native-dev",
            .local_name = "Native Player",
        },
        .slots = {
            { .kind = Gm_PKind_Human, .character = CKind_Falco,
              .cpu_level = 1, .team = 0, .handicap = 9 },
            { .kind = Gm_PKind_Cpu, .character = CKind_Luigi,
              .cpu_level = 1, .team = 1, .handicap = 9 },
            { .kind = Gm_PKind_NA, .cpu_level = 1, .team = 0,
              .handicap = 9 },
            { .kind = Gm_PKind_NA, .cpu_level = 1, .team = 1,
              .handicap = 9 },
        },
    };
    *iso_path = NULL;
    bool has_match_option = false;
    for (int i = 1; i < argc; ++i) {
        const char* key = argv[i];
        if (strcmp(key, "--help") == 0) {
            usage(argv[0]);
            return 0;
        }
        if (strcmp(key, "--retail-menu") == 0) { launch.retail_menu = true; continue; }
        if (strcmp(key, "--match") == 0) {
            launch.direct = true;
            continue;
        }
        if (strcmp(key, "--fast") == 0) {
            launch.fast = true;
            continue;
        }
        if (strcmp(key, "--mute") == 0) {
            launch.muted = true;
            continue;
        }
        if (strcmp(key, "--hidden") == 0) {
            launch.hidden = true;
            continue;
        }
        if (strcmp(key, "--trace-actions") == 0) {
            launch.trace_actions = true;
            continue;
        }
        if (strcmp(key, "--trace-fighter-state") == 0) {
            launch.trace_fighter_state = true;
            continue;
        }
        if (strcmp(key, "--rollback-census") == 0) {
            launch.rollback_census = true;
            continue;
        }
        if (strcmp(key, "--object-census") == 0) {
            launch.object_census = true;
            continue;
        }
        if (strcmp(key, "--snapshot-census") == 0) {
            launch.snapshot_census = true;
            continue;
        }
        if (strcmp(key, "--rollback-probe") == 0) {
            launch.rollback_probe = true;
            continue;
        }
        if (strcmp(key, "--rollback-benchmark") == 0) {
            launch.rollback_benchmark = true;
            continue;
        }
        if (strcmp(key, "--rollback-correction-probe") == 0) {
            launch.rollback_correction_probe = true;
            continue;
        }
        if (strcmp(key, "--teams") == 0) {
            launch.teams = true;
            has_match_option = true;
            continue;
        }
        if (i + 1 >= argc) {
            fprintf(stderr, "Missing value for %s\n", key);
            usage(argv[0]);
            return -1;
        }
        const char* value = argv[++i];
        if (strcmp(key, "--trace-geometry-frame") == 0) {
            if (!parse_u64(value, UINT64_MAX - 1, &launch.trace_geometry_frame)) {
                fprintf(stderr, "Invalid geometry trace frame: %s\n", value);
                return -1;
            }
            launch.trace_geometry = true;
            launch.trace_fighter_state = true;
            continue;
        }
        if (strcmp(key, "--rollback-correction-frame") == 0) {
            unsigned frame;
            if (!parse_number(value, 240, 591, &frame)) {
                fprintf(stderr, "Invalid rollback correction frame: %s\n",
                        value);
                return -1;
            }
            launch.rollback_correction_frame = frame;
            launch.rollback_correction_frame_explicit = true;
            continue;
        }
        bool valid = false;
        if (strcmp(key, "--iso") == 0) {
            valid = *iso_path == NULL;
            *iso_path = value;
        } else if (strcmp(key, "--content-root") == 0) {
            valid = launch.content_root == NULL && value[0] != 0;
            if (valid) {
                launch.content_root = value;
            }
        } else if (strcmp(key, "--record") == 0 ||
                   strcmp(key, "--replay") == 0 ||
                   strcmp(key, "--replay-inputs") == 0)
        {
            valid = launch.replay_path == NULL && value[0] != 0;
            if (valid) {
                launch.replay_path = value;
                launch.replay_playback = strcmp(key, "--record") != 0;
                launch.replay_inputs_only =
                    strcmp(key, "--replay-inputs") == 0;
            }
        } else if (strcmp(key, "--netplay-session") == 0) {
            valid = !launch.netplay_session_explicit &&
                    parse_u64(value, UINT64_MAX, &launch.netplay.session_id);
            launch.netplay_session_explicit = valid;
        } else if (strcmp(key, "--netplay-listen") == 0) {
            unsigned port;
            valid = !launch.netplay_listen_explicit &&
                    parse_number(value, 1, UINT16_MAX, &port);
            if (valid) {
                launch.netplay.listen_port = (uint16_t) port;
                launch.netplay_listen_explicit = true;
            }
        } else if (strcmp(key, "--netplay-local-peer") == 0) {
            unsigned peer;
            valid = !launch.netplay_local_peer_explicit &&
                    parse_number(value, 0, 3, &peer);
            if (valid) {
                launch.netplay.local_peer = (uint8_t) peer;
                launch.netplay_local_peer_explicit = true;
            }
        } else if (strcmp(key, "--netplay-local-mask") == 0) {
            uint64_t mask;
            valid = !launch.netplay_local_mask_explicit &&
                    parse_u64(value, 0xF, &mask) && mask != 0;
            if (valid) {
                launch.netplay.local_port_mask = (uint8_t) mask;
                launch.netplay_local_mask_explicit = true;
            }
        } else if (strcmp(key, "--netplay-peer") == 0) {
            valid =
                launch.netplay.peer_count <
                    sizeof(launch.netplay.peers) /
                        sizeof(launch.netplay.peers[0]) &&
                parse_netplay_peer(
                    value, &launch.netplay.peers[launch.netplay.peer_count]);
            if (valid) {
                ++launch.netplay.peer_count;
            }
        } else if (strcmp(key, "--netplay-room") == 0) {
            valid = !launch.netplay_room_explicit &&
                    copy_option(launch.netplay.room.signaling_endpoint,
                                sizeof(launch.netplay.room.signaling_endpoint),
                                value);
            launch.netplay_room_explicit = valid;
        } else if (strcmp(key, "--netplay-room-id") == 0) {
            valid = !launch.netplay_room_id_explicit &&
                    copy_option(launch.netplay.room.room_id,
                                sizeof(launch.netplay.room.room_id), value);
            launch.netplay_room_id_explicit = valid;
        } else if (strcmp(key, "--netplay-peer-id") == 0) {
            valid = !launch.netplay_peer_id_explicit &&
                    copy_option(launch.netplay.room.local_peer_id,
                                sizeof(launch.netplay.room.local_peer_id),
                                value);
            launch.netplay_peer_id_explicit = valid;
        } else if (strcmp(key, "--netplay-room-peer") == 0) {
            valid =
                launch.netplay.room.peer_count <
                    sizeof(launch.netplay.room.peers) /
                        sizeof(launch.netplay.room.peers[0]) &&
                parse_netplay_room_peer(
                    value,
                    &launch.netplay.room
                         .peers[launch.netplay.room.peer_count]);
            if (valid) {
                ++launch.netplay.room.peer_count;
            }
        } else if (strcmp(key, "--netplay-stun") == 0) {
            valid = !launch.netplay_stun_explicit &&
                    parse_host_port(value, launch.netplay.room.stun_host,
                                    sizeof(launch.netplay.room.stun_host),
                                    &launch.netplay.room.stun_port);
            launch.netplay_stun_explicit = valid;
        } else if (strcmp(key, "--netplay-delay") == 0) {
            unsigned frames;
            valid = !launch.netplay_delay_explicit &&
                    parse_number(value, 0,
                                 NativeConfirmedCheckpoint_MaxPrediction - 1,
                                 &frames);
            if (valid) {
                launch.artificial_delay_frames = (uint8_t) frames;
                launch.netplay_delay_explicit = true;
            }
        } else if (strcmp(key, "--netplay-input-delay") == 0) {
            unsigned frames;
            valid = !launch.input_delay_explicit &&
                    parse_number(value, 0, NATIVE_NETPLAY_MAX_INPUT_DELAY, &frames);
            if (valid) {
                launch.input_delay_frames = (uint8_t) frames;
                launch.input_delay_explicit = true;
            }
        } else if (strcmp(key,"--script-kind")==0) {
            unsigned long number;char* end;
            number=strtoul(value,&end,0);
            valid=end!=value && !*end && number<Ft_Kind_RuntimeCapacity && number!=Ft_Kind_None;
            if(valid)launch.script_kind=value;
        } else if (strcmp(key,"--script-source")==0) {
            valid=*value!=0;if(valid)launch.script_source=value;
        } else if (strcmp(key, "--stage") == 0) {
            valid = parse_stage(value, &launch.stage);
            has_match_option = true;
        } else if (strcmp(key, "--mode") == 0) {
            if (option_is(value, "time")) {
                launch.mode = MatchKind_Time;
                valid = true;
            } else if (option_is(value, "stock")) {
                launch.mode = MatchKind_Stock;
                valid = true;
            } else if (option_is(value, "coin")) {
                launch.mode = MatchKind_Coin;
                valid = true;
            }
            has_match_option = true;
        } else if (strcmp(key, "--time") == 0) {
            unsigned minutes;
            valid = parse_number(value, 0, 99, &minutes);
            launch.time_minutes = (u8) minutes;
            launch.time_explicit = true;
            has_match_option = true;
        } else if (strcmp(key, "--stocks") == 0) {
            unsigned stocks;
            valid = parse_number(value, 1, 99, &stocks);
            launch.stocks = (u8) stocks;
            has_match_option = true;
        } else if (strcmp(key, "--items") == 0) {
            valid = parse_items(value, &launch.item_freq);
            has_match_option = true;
        } else if (strcmp(key, "--fixture-item") == 0) {
            valid = nativeMatchFixture_AddItem(value, false);
            has_match_option = true;
        } else if (strcmp(key, "--fixture-capturable-item") == 0) {
            valid = nativeMatchFixture_AddItem(value, true);
            has_match_option = true;
        } else if (strcmp(key, "--fixture-airborne") == 0) {
            valid = nativeMatchFixture_AddAirborne(value);
            has_match_option = true;
        } else if (strlen(key) == 4 && strncmp(key, "--p", 3) == 0 &&
                   key[3] >= '1' && key[3] <= '4')
        {
            valid = parse_slot(value, &launch.slots[key[3] - '1']);
            has_match_option = true;
        } else {
            int player;
            if (parse_player_option(key, "-color", &player)) {
                unsigned color;
                valid = parse_number(value, 0, UINT8_MAX, &color);
                if (valid) {
                    launch.slots[player].color = (u8) color;
                    launch.slots[player].color_explicit = true;
                }
                has_match_option = true;
            } else if (parse_player_option(key, "-team", &player)) {
                valid = parse_team(value, &launch.slots[player].team);
                launch.slots[player].team_explicit = valid;
                has_match_option = true;
            } else if (parse_player_option(key, "-profile", &player)) {
                valid = !launch.profile_explicit[player] &&
                        nativeProfileStore_IsValidId(value) &&
                        copy_option(launch.profile_ids[player],
                                    sizeof(launch.profile_ids[player]), value);
                launch.profile_explicit[player] = valid;
            } else if (parse_player_option(key, "-skin", &player)) {
                valid = !launch.costume_package_explicit[player] &&
                        nativeContentStore_IdFromHex(
                            value, &launch.costume_packages[player]);
                launch.costume_package_explicit[player] = valid;
                has_match_option = true;
            } else if (parse_player_option(key, "-fighter", &player)) {
                valid = !launch.fighter_package_explicit[player] &&
                        nativeContentStore_IdFromHex(
                            value, &launch.fighter_packages[player]);
                launch.fighter_package_explicit[player] = valid;
                has_match_option = true;
            }
        }
        if (!valid) {
            fprintf(stderr, "Invalid %s value: %s\n", key, value);
            usage(argv[0]);
            return -1;
        }
    }
    if (*iso_path == NULL || (has_match_option && !launch.direct)) {
        fprintf(stderr,
                "An ISO is required; match options require --match.\n");
        usage(argv[0]);
        return -1;
    }
    if (launch.retail_menu && launch.direct) {
        fprintf(stderr, "Choose --retail-menu or --match.\n");
        return -1;
    }
    if (launch.replay_path != NULL && !launch.direct) {
        fprintf(stderr, "Recording and replay currently require --match.\n");
        return -1;
    }
    if (launch.fast && !launch.direct) {
        fprintf(stderr, "--fast requires --match.\n");
        return -1;
    }
    if (launch.trace_actions && !launch.direct) {
        fprintf(stderr, "--trace-actions requires --match.\n");
        return -1;
    }
    if (launch.trace_fighter_state &&
        (!launch.direct || launch.replay_path == NULL)) {
        fprintf(stderr,
                "--trace-fighter-state requires --match and a recording or replay.\n");
        return -1;
    }
    if (launch.rollback_census && !launch.direct) {
        fprintf(stderr, "--rollback-census requires --match.\n");
        return -1;
    }
    if ((launch.rollback_probe || launch.rollback_correction_probe || launch.rollback_benchmark) &&
        (!launch.direct || launch.replay_path == NULL ||
         !launch.replay_playback))
    {
        fprintf(stderr, "Rollback probes require --match and --replay or "
                        "--replay-inputs.\n");
        return -1;
    }
    bool has_direct_netplay =
        launch.netplay_listen_explicit || launch.netplay.peer_count != 0;
    bool has_room_netplay =
        launch.netplay_room_explicit || launch.netplay_room_id_explicit ||
        launch.netplay_peer_id_explicit || launch.netplay_stun_explicit ||
        launch.netplay.room.peer_count != 0;
    bool has_netplay =
        launch.netplay_session_explicit ||
        launch.netplay_local_peer_explicit ||
        launch.netplay_local_mask_explicit ||
        (launch.direct && launch.netplay_delay_explicit) ||
        has_direct_netplay || has_room_netplay;
    if (nativeMatchFixture_Configured() &&
        (!launch.direct || !launch.replay_inputs_only || has_netplay ||
         launch.rollback_probe || launch.rollback_correction_probe || launch.rollback_benchmark)) {
        fprintf(stderr, "Match fixtures require an offline --match --replay-inputs without rollback probes.\n");
        return -1;
    }
    if (has_netplay) {
        if (launch.replay_inputs_only) {
            fprintf(stderr, "Input comparison playback is local only.\n");
            return -1;
        }
        if (!launch.direct || !launch.netplay_session_explicit ||
            !launch.netplay_local_peer_explicit ||
            !launch.netplay_local_mask_explicit ||
            has_direct_netplay == has_room_netplay)
        {
            fprintf(stderr,
                    "Netplay requires --match, common ownership settings, "
                    "and exactly one transport.\n");
            return -1;
        }
        if (has_direct_netplay &&
            (!launch.netplay_listen_explicit ||
             launch.netplay.peer_count == 0))
        {
            fprintf(stderr, "Raw UDP netplay requires a listen port and "
                            "at least one peer.\n");
            return -1;
        }
        if (has_room_netplay &&
            (!launch.netplay_room_explicit ||
             !launch.netplay_room_id_explicit ||
             !launch.netplay_peer_id_explicit ||
             launch.netplay.room.peer_count == 0))
        {
            fprintf(stderr, "Room netplay requires an origin, room ID, local "
                            "peer ID, and at least one room peer.\n");
            return -1;
        }
        if (launch.fast || launch.rollback_census || launch.object_census || launch.rollback_probe || launch.rollback_benchmark ||
            launch.rollback_correction_probe)
        {
            fprintf(stderr, "Netplay cannot be combined with fast or rollback "
                            "diagnostics.\n");
            return -1;
        }
        uint8_t assigned_mask = launch.netplay.local_port_mask;
        uint8_t peer_mask = 1U << launch.netplay.local_peer;
        uint32_t remote_count = has_room_netplay
                                    ? launch.netplay.room.peer_count
                                    : launch.netplay.peer_count;
        for (uint32_t i = 0; i < remote_count; ++i) {
            uint8_t remote_peer =
                has_room_netplay ? launch.netplay.room.peers[i].peer
                                 : launch.netplay.peers[i].peer;
            uint8_t remote_mask =
                has_room_netplay ? launch.netplay.room.peers[i].port_mask
                                 : launch.netplay.peers[i].port_mask;
            if ((peer_mask & (1U << remote_peer)) != 0 ||
                (assigned_mask & remote_mask) != 0)
            {
                fprintf(stderr,
                        "Netplay peer or fighter ownership overlaps.\n");
                return -1;
            }
            if (has_room_netplay) {
                const char* remote_id =
                    launch.netplay.room.peers[i].peer_id;
                if (strcmp(remote_id, launch.netplay.room.local_peer_id) == 0)
                {
                    fprintf(stderr,
                            "A room peer ID cannot be the local peer ID.\n");
                    return -1;
                }
                for (uint32_t prior = 0; prior < i; ++prior) {
                    if (strcmp(remote_id,
                               launch.netplay.room.peers[prior].peer_id) == 0)
                    {
                        fprintf(stderr, "Room peer IDs must be unique.\n");
                        return -1;
                    }
                }
            }
            peer_mask |= 1U << remote_peer;
            assigned_mask |= remote_mask;
        }
        if (assigned_mask != 0xF) {
            fprintf(stderr,
                    "Netplay peers must assign all four controller ports.\n");
            return -1;
        }
        launch.netplay.room.enabled = has_room_netplay;
        launch.netplay.artificial_delay_frames = launch.artificial_delay_frames;
        launch.netplay.enabled = true;
    }
    if (launch.stage == St_Kind_Practice && launch.netplay.enabled) {
        fprintf(stderr, "The practice stage is local only.\n");
        return -1;
    }
    if (launch.rollback_probe && launch.rollback_correction_probe) {
        fprintf(stderr, "Select one rollback probe at a time.\n");
        return -1;
    }
    if (launch.object_census && launch.rollback_benchmark) {
        fprintf(stderr, "--object-census is untimed and cannot accompany --rollback-benchmark.\n");
        return -1;
    }
    if (launch.snapshot_census && (launch.netplay.enabled ||
        launch.rollback_benchmark || launch.rollback_probe ||
        launch.rollback_correction_probe || launch.rollback_census)) {
        fprintf(stderr, "--snapshot-census is offline only and cannot accompany rollback diagnostics.\n");
        return -1;
    }
    if (launch.rollback_benchmark) {
        if (launch.rollback_probe || launch.rollback_correction_probe || launch.rollback_census ||
            launch.trace_actions || launch.trace_fighter_state || !launch.hidden || !launch.muted ||
            launch.mode != MatchKind_Stock || launch.stocks != 99) {
            fprintf(stderr, "--rollback-benchmark requires hidden muted 99-stock playback without other diagnostics.\n");
            return -1;
        }
        for (unsigned port = 0; port < 4; ++port) {
            if (launch.slots[port].kind != Gm_PKind_Cpu) {
                fprintf(stderr, "--rollback-benchmark requires four CPU slots; live paired actors are validated during measurement.\n");
                return -1;
            }
        }
    }
    if (launch.rollback_correction_frame_explicit &&
        !launch.rollback_correction_probe) {
        fprintf(stderr, "--rollback-correction-frame requires "
                        "--rollback-correction-probe.\n");
        return -1;
    }
    if ((launch.rollback_probe || launch.rollback_correction_probe) &&
        launch.rollback_census)
    {
        fprintf(
            stderr,
            "Rollback probes and --rollback-census cannot run together.\n");
        return -1;
    }
    if (!resolve_direct_fighters()) {
        return -1;
    }
    if (launch.direct) {
        int active = 0;
        int human = 0;
        unsigned team_mask = 0;
        for (int i = 0; i < 4; ++i) {
            active += launch.slots[i].kind != Gm_PKind_NA;
            human += launch.slots[i].kind == Gm_PKind_Human;
            if (launch.slots[i].kind == Gm_PKind_NA) {
                if (launch.slots[i].color_explicit ||
                    launch.slots[i].team_explicit)
                {
                    fprintf(stderr, "P%d is off but has a color or team.\n",
                            i + 1);
                    return -1;
                }
                continue;
            }
            if (launch.slots[i].color_explicit) {
                if (launch.teams) {
                    fprintf(
                        stderr,
                        "P%d cannot select an FFA costume in team battle.\n",
                        i + 1);
                    return -1;
                }
                if (launch.slots[i].color >=
                    gm_GetNumCostumesForCKind(launch.slots[i].character))
                {
                    fprintf(
                        stderr,
                        "P%d costume %u is unavailable for that fighter.\n",
                        i + 1, launch.slots[i].color);
                    return -1;
                }
            }
            if (launch.slots[i].team_explicit && !launch.teams) {
                fprintf(stderr, "P%d team requires --teams.\n", i + 1);
                return -1;
            }
            team_mask |= 1U << launch.slots[i].team;
        }
        bool practice = launch.stage == St_Kind_Practice;
        if (active < (practice ? 1 : 2) ||
            (human == 0 && !launch.rollback_benchmark && !launch.object_census &&
             !launch.snapshot_census) ||
            (!practice && launch.mode != MatchKind_Stock &&
             launch.time_minutes == 0))
        {
            fprintf(stderr, practice
                ? "Practice needs at least one human fighter.\n"
                : "A direct match needs at least two fighters, one human, "
                  "and a time limit for time/coin rules.\n");
            return -1;
        }
        if (launch.teams && (team_mask & (team_mask - 1)) == 0) {
            fprintf(stderr,
                    "A team battle needs fighters on at least two teams.\n");
            return -1;
        }
        if (has_room_netplay) {
            uint8_t human_ports = 0;
            for (uint32_t port = 0; port < 4; ++port) {
                if (launch.slots[port].kind == Gm_PKind_Human) {
                    human_ports |= 1U << port;
                }
            }
            bool every_peer_has_human =
                (launch.netplay.local_port_mask & human_ports) != 0;
            for (uint32_t i = 0;
                 every_peer_has_human &&
                 i < launch.netplay.room.peer_count; ++i)
            {
                every_peer_has_human =
                    (launch.netplay.room.peers[i].port_mask & human_ports) !=
                    0;
            }
            if (!every_peer_has_human) {
                fprintf(stderr, "Each active room peer must own at least one "
                                "human fighter.\n");
                return -1;
            }
        }
    }
    for (unsigned port = 0; port < 4; ++port) {
        if (!launch.costume_package_explicit[port]) {
            continue;
        }
        if ((launch.netplay.enabled && !launch.netplay.room.enabled) ||
            launch.slots[port].kind == Gm_PKind_NA) {
            fprintf(stderr, "P%u skin needs an active local or room slot.\n",
                    port + 1);
            return -1;
        }
        uint8_t color = launch.teams
                            ? team_costume(launch.slots[port].character,
                                           launch.slots[port].team)
                            : launch.slots[port].color;
        const NativeContentId empty_id = { 0 };
        bool custom = memcmp(&launch.fighter_packages[port], &empty_id,
                             sizeof(empty_id)) != 0;
        bool loaded = custom
            ? nativeCostumeRuntime_LoadCustom(
                  port, &launch.costume_packages[port],
                  &launch.fighter_packages[port],
                  Ft_Kind_CustomFirst +
                      (launch.slots[port].character - ChKind_CustomFirst) * NATIVE_FIGHTER_FAMILY_MEMBERS,
                  launch.content_root, true)
            : nativeCostumeRuntime_LoadInstalled(
                  port, &launch.costume_packages[port],
                  launch.slots[port].character, color, launch.content_root);
        if (!loaded) {
            fprintf(stderr,
                    "P%u skin is missing, not installed, or incompatible "
                    "with the selected fighter/color.\n", port + 1);
            return -1;
        }
    }
    if ((launch.script_kind!=NULL)!=(launch.script_source!=NULL) ||
        (launch.script_source && (!launch.direct || launch.netplay.enabled))) {
        fprintf(stderr,"--script-kind and --script-source require an offline direct match together.\n");
        return -1;
    }
    return 1;
}

bool nativeMatchLaunch_RetailMenu(void) { return launch.retail_menu; }

bool nativeMatchLaunch_IsDirect(void)
{
    return launch.direct;
}

const char* nativeMatchLaunch_ScriptKind(void){return launch.script_kind;}
const char* nativeMatchLaunch_ScriptSource(void){return launch.script_source;}

bool nativeMatchLaunch_PrepareDirectFighters(void)
{
    if (!launch.direct) return false;
    if (launch.direct_prepared) return true;
    if (!nativeFighterRuntime_StageCached(launch.fighter_ids,
                                          launch.fighter_count,
                                          launch.content_root, true)) {
        fputs("Could not prepare selected custom fighters.\n", stderr);
        launch.direct_prepare_failed = true;
        return false;
    }
    launch.direct_prepared = true;
    return true;
}

bool nativeMatchLaunch_DirectPreparationFailed(void)
{
    return launch.direct_prepare_failed;
}

uint8_t nativeMatchLaunch_ArtificialDelayFrames(void)
{
    return launch.artificial_delay_frames;
}

uint8_t nativeMatchLaunch_InputDelayFrames(bool online)
{
    if (launch.input_delay_explicit) return launch.input_delay_frames;
    return online && !nativeMatchLaunch_ReplayPlayback() ? 2 : 0;
}

bool nativeMatchLaunch_PrepareSuddenDeathFighters(void)
{
    if (!nativeFighterRuntime_StageCached(
            launch.fighter_ids, launch.fighter_count, launch.content_root,
            !launch.netplay.enabled && !launch.local_preview && !launch.spectating)) {
        return false;
    }
    if (launch.direct) launch.direct_prepared = true;
    return true;
}

const char* nativeMatchLaunch_ContentRoot(void)
{
    return launch.content_root;
}

bool nativeMatchLaunch_ApplyProfiles(void)
{
    nativeProfileInput_ClearAll();
    for (uint32_t port = 0; port < 4; ++port) {
        NativeProfile profile;
        if (!launch.profile_explicit[port]) {
            char id[NATIVE_PROFILE_ID_BYTES + 1];
            if (nativeProfileStore_ReadSelection(port, id) && *id &&
                nativeProfileStore_Load(&profile, id)) {
                nativeProfileInput_Assign(port, &profile);
            }
            continue;
        }
        if (!nativeProfileStore_Load(&profile, launch.profile_ids[port])) {
            fprintf(stderr, "Could not load P%u profile: %s\n", port + 1,
                    launch.profile_ids[port]);
            nativeProfileInput_ClearAll();
            return false;
        }
        if (!nativeProfileInput_Assign(port, &profile)) {
            fprintf(stderr, "Could not apply P%u profile: %s\n", port + 1,
                    launch.profile_ids[port]);
            nativeProfileInput_ClearAll();
            return false;
        }
    }
    return true;
}

const char* nativeMatchLaunch_ReplayPath(void)
{
    return launch.replay_path;
}

bool nativeMatchLaunch_ReplayPlayback(void)
{
    return launch.replay_playback;
}

bool nativeMatchLaunch_ReplayInputsOnly(void)
{
    return launch.replay_inputs_only;
}

bool nativeMatchLaunch_Muted(void)
{
    return launch.muted;
}

bool nativeMatchLaunch_Hidden(void)
{
    return launch.hidden;
}

bool nativeMatchLaunch_Fast(void)
{
    return launch.fast || launch.rollback_probe || launch.rollback_benchmark ||
           launch.rollback_correction_probe;
}

bool nativeMatchLaunch_TraceActions(void)
{
    return launch.trace_actions;
}

bool nativeMatchLaunch_TraceFighterState(void)
{
    return launch.trace_fighter_state;
}

uint64_t nativeMatchLaunch_TraceGeometryFrame(void)
{
    return launch.trace_geometry ? launch.trace_geometry_frame : UINT64_MAX;
}

bool nativeMatchLaunch_RollbackCensus(void)
{
    return launch.rollback_census;
}

bool nativeMatchLaunch_ObjectCensus(void)
{
    return launch.object_census;
}

bool nativeMatchLaunch_SnapshotCensus(void)
{
    return launch.snapshot_census;
}

bool nativeMatchLaunch_RollbackBenchmark(void)
{
    return launch.rollback_benchmark;
}

bool nativeMatchLaunch_RollbackProbe(void)
{
    return launch.rollback_probe || launch.rollback_correction_probe;
}

bool nativeMatchLaunch_RollbackCorrectionProbe(void)
{
    return launch.rollback_correction_probe;
}

unsigned nativeMatchLaunch_RollbackCorrectionFrame(void)
{
    return launch.rollback_correction_frame_explicit
        ? launch.rollback_correction_frame : 240;
}

const NativeNetplayLaunchConfig* nativeMatchLaunch_Netplay(void)
{
    return launch.netplay.enabled ? &launch.netplay : NULL;
}

void nativeMatchLaunch_ClearNetplay(void)
{
    launch.netplay = (NativeNetplayLaunchConfig){ 0 };
    launch.has_agreed_match = false;
    launch.local_preview = false;
    launch.spectating = false;
}

uint64_t nativeMatchLaunch_Signature(void)
{
    if (launch.has_agreed_match) {
        uint8_t bytes[NATIVE_LOBBY_MATCH_MAX_WIRE_BYTES];
        size_t size = nativeLobbyMatch_Encode(&launch.agreed_match, bytes,
                                              sizeof(bytes));
        uint64_t hash = UINT64_C(14695981039346656037);
        for (size_t i = 0; i < size; ++i) {
            hash ^= bytes[i];
            hash *= UINT64_C(1099511628211);
        }
        return hash;
    }
    // Hash only semantic settings, not struct padding or argument spelling.
    const u8 settings[] = {
        launch.direct,
        launch.time_explicit,
        launch.teams,
        (u8) launch.stage,
        launch.mode,
        launch.time_minutes,
        launch.stocks,
        (u8) launch.item_freq,
        launch.slots[0].kind,
        launch.slots[0].character,
        launch.slots[0].cpu_level,
        launch.slots[0].color,
        launch.slots[0].team,
        launch.slots[0].handicap,
        launch.slots[1].kind,
        launch.slots[1].character,
        launch.slots[1].cpu_level,
        launch.slots[1].color,
        launch.slots[1].team,
        launch.slots[1].handicap,
        launch.slots[2].kind,
        launch.slots[2].character,
        launch.slots[2].cpu_level,
        launch.slots[2].color,
        launch.slots[2].team,
        launch.slots[2].handicap,
        launch.slots[3].kind,
        launch.slots[3].character,
        launch.slots[3].cpu_level,
        launch.slots[3].color,
        launch.slots[3].team,
        launch.slots[3].handicap,
    };
    uint64_t hash = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < sizeof(settings); ++i) {
        hash ^= settings[i];
        hash *= UINT64_C(1099511628211);
    }
    const uint8_t* stock_digest = nativeStockContent_Digest();
    for (size_t i = 0; i < NATIVE_STOCK_CONTENT_DIGEST_BYTES; ++i) {
        hash ^= stock_digest[i];
        hash *= UINT64_C(1099511628211);
    }
    for (unsigned port = 0; port < 4; ++port) {
        for (unsigned byte = 0; byte < NATIVE_CONTENT_ID_BYTES; ++byte) {
            hash ^= launch.costume_packages[port].bytes[byte];
            hash *= UINT64_C(1099511628211);
        }
        for (unsigned byte = 0; byte < NATIVE_CONTENT_ID_BYTES; ++byte) {
            hash ^= launch.fighter_packages[port].bytes[byte];
            hash *= UINT64_C(1099511628211);
        }
    }
    if (launch.stage > UINT8_MAX) {
        hash ^= launch.stage >> 8;
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

bool nativeMatchLaunch_CreateLobbyMatch(NativeLobbyMatch* match)
{
    if (match == NULL || !launch.netplay.enabled ||
        !launch.netplay.room.enabled)
    {
        return false;
    }
    if (!nativeMatchSelection_Build(match, gmVsMelee_GetVsData(),
                                    launch.stage, launch.costume_packages,
                                    launch.fighter_packages))
    {
        return false;
    }
    match->session_id = launch.netplay.session_id;
    match->peer_count = launch.netplay.room.peer_count + 1;

    uint8_t peer_index = 0;
    for (uint8_t numeric_peer = 0; numeric_peer < 4; ++numeric_peer) {
        NativeLobbyMatchPeer* output = &match->peers[peer_index];
        const char* peer_id = NULL;
        uint8_t port_mask = 0;
        if (launch.netplay.local_peer == numeric_peer) {
            peer_id = launch.netplay.room.local_peer_id;
            port_mask = launch.netplay.local_port_mask;
        } else {
            for (uint32_t remote = 0;
                 remote < launch.netplay.room.peer_count; ++remote)
            {
                const NativeNetplayRoomPeerLaunch* candidate =
                    &launch.netplay.room.peers[remote];
                if (candidate->peer == numeric_peer) {
                    peer_id = candidate->peer_id;
                    port_mask = candidate->port_mask;
                    break;
                }
            }
        }
        if (peer_id == NULL) {
            continue;
        }
        size_t id_length = strnlen(peer_id, sizeof(output->peer_id));
        if (id_length == 0 || id_length >= sizeof(output->peer_id)) {
            return false;
        }
        memcpy(output->peer_id, peer_id, id_length + 1);
        output->numeric_peer = numeric_peer;
        output->port_mask = port_mask;
        ++peer_index;
    }
    if (peer_index != match->peer_count) {
        return false;
    }

    return nativeLobbyMatch_Validate(match);
}

static bool prepare_match(const NativeLobbyMatch* match)
{
    if (!nativeLobbyMatch_Validate(match) ||
        memcmp(match->content_digest, nativeStockContent_Digest(),
               NATIVE_STOCK_CONTENT_DIGEST_BYTES) != 0)
    {
        return false;
    }

    uint8_t stage;
    if (!nativeStockContent_StageFromId(match->stage_id, &stage)) {
        return false;
    }
    NativeContentId fighter_packages[4];
    for (unsigned port = 0; port < 4; ++port) {
        fighter_packages[port] = match->slots[port].character_package_id;
    }
    uint8_t custom_kinds[4];
    if (!resolve_fighter_packages(fighter_packages, custom_kinds, false,
                                  true)) {
        return false;
    }
    NativeMatchSlot slots[4] = { 0 };
    nativeCostumeRuntime_Clear();
    for (uint32_t port = 0; port < 4; ++port) {
        const NativeLobbyMatchSlot* source = &match->slots[port];
        NativeMatchSlot* output = &slots[port];
        const NativeContentId no_package = { 0 };
        if (source->kind == NativeLobbyMatchSlot_Empty) {
            output->kind = Gm_PKind_NA;
            continue;
        }
        bool custom = memcmp(&source->character_package_id, &no_package,
                             sizeof(no_package)) != 0;
        if (custom) {
            if (source->character_id != 0 || source->costume_id != 0 ||
                custom_kinds[port] < ChKind_CustomFirst) {
                return false;
            }
            output->character = custom_kinds[port];
            output->color = 0;
            if (memcmp(&source->costume_package_id, &no_package,
                       sizeof(no_package)) != 0 &&
                !nativeCostumeRuntime_LoadCustom(
                    port, &source->costume_package_id,
                    &source->character_package_id,
                    Ft_Kind_CustomFirst +
                        (custom_kinds[port] - ChKind_CustomFirst) * NATIVE_FIGHTER_FAMILY_MEMBERS,
                    launch.content_root, false)) {
                return false;
            }
        } else {
            if (!nativeStockContent_CharacterFromId(source->character_id,
                                         &output->character) ||
                !nativeStockContent_CostumeFromId(source->costume_id,
                                         output->character, &output->color)) {
                return false;
            }
            if (memcmp(&source->costume_package_id, &no_package,
                       sizeof(no_package)) != 0 &&
                !nativeCostumeRuntime_LoadCached(
                    port, &source->costume_package_id, output->character,
                    output->color, launch.content_root)) {
                return false;
            }
        }
        output->kind = source->kind == NativeLobbyMatchSlot_Human
                           ? Gm_PKind_Human
                           : Gm_PKind_Cpu;
        output->cpu_level = source->cpu_level;
        output->team = source->team;
        output->handicap = source->handicap;
        output->color_explicit = !match->teams;
        output->team_explicit = match->teams;
        if (match->teams && !custom && output->color !=
                                team_costume(output->character, output->team))
        {
            return false;
        }
    }

    launch.stage = stage;
    switch (match->mode) {
    case NativeLobbyMatchMode_Time:
        launch.mode = MatchKind_Time;
        break;
    case NativeLobbyMatchMode_Stock:
        launch.mode = MatchKind_Stock;
        break;
    case NativeLobbyMatchMode_Coin:
        launch.mode = MatchKind_Coin;
        break;
    default:
        return false;
    }
    launch.time_minutes = match->time_limit_minutes;
    launch.time_explicit = true;
    launch.stocks = match->stock_count;
    launch.item_freq = match->item_frequency;
    launch.teams = match->teams;
    memcpy(launch.slots, slots, sizeof(slots));
    launch.agreed_match = *match;
    launch.has_agreed_match = true;

    GameRules* rules = gmMainLib_GetGameRules();
    struct GamePrefs* prefs = gmMainLib_GetGamePrefs();
    if (!nativeSettings_PreserveForRoomMatch(rules, prefs)) {
        return false;
    }
    // The match description owns every slot. CSS and practice can retain
    // character IDs and spawn overrides even in slots marked empty.
    gm_SetupAllPlayerDefaults(gmVsMelee_GetVsData()->start.players);
    nativeMatchLaunch_Apply();
    rules->time_limit = match->time_limit_minutes;
    rules->stock_time_limit = match->stock_time_limit_minutes;
    rules->handicap = match->handicap;
    rules->damage_ratio = match->damage_ratio;
    rules->friendly_fire = match->friendly_fire;
    rules->pause = match->pause;
    rules->score_display = match->score_display;
    rules->unk_xc = match->sd_penalty;
    prefs->item_freq = match->item_frequency < 0
                           ? UINT8_MAX
                           : (uint8_t) match->item_frequency;
    prefs->item_mask = match->item_mask;
    return true;
}

bool nativeMatchLaunch_AdoptLobbyRoom(const NativeNetplayRoom* room)
{
    if(!room || !room->active || !room->match_configured || !room->ready)return false;
    const NativeLobbyMatch* match=&room->match;
    const NativeLobbyMatchPeer* local = NULL;
    for (uint32_t i = 0; i < match->peer_count; ++i) {
        if (strcmp(match->peers[i].peer_id,
                   room->connection.local_peer_id) == 0)
        {
            local = &match->peers[i];
            break;
        }
    }
    if (local == NULL || match->peer_count < 2) {
        return false;
    }

    NativeNetplayLaunchConfig netplay = {
        .session_id = match->session_id,
        .local_peer = local->numeric_peer,
        .local_port_mask = local->port_mask,
        .artificial_delay_frames = room->artificial_delay_frames,
        .enabled = true,
        .room = {
            .stun_port = room->connection.ice_config.stun_port,
            .enabled = true,
        },
    };
    for (uint32_t port = 0; port < 4; ++port) {
        netplay.local_seats[port] = match->slots[port].local_seat;
    }
    const NativeSignalingClient* signaling = &room->connection.signaling;
    if (!copy_option(netplay.room.signaling_endpoint,
                     sizeof(netplay.room.signaling_endpoint),
                     signaling->endpoint) ||
        !copy_option(netplay.room.room_id, sizeof(netplay.room.room_id),
                     signaling->room_id) ||
        !copy_option(netplay.room.local_peer_id,
                     sizeof(netplay.room.local_peer_id),
                     room->connection.local_peer_id) ||
        !copy_option(netplay.room.local_name,
                     sizeof(netplay.room.local_name), signaling->name) ||
        !copy_option(netplay.room.build, sizeof(netplay.room.build),
                     room->connection.local_build) ||
        !copy_option(netplay.room.stun_host,
                     sizeof(netplay.room.stun_host),
                     room->connection.stun_host))
    {
        return false;
    }

    NativeNetplayRoomPeerLaunch peers[3] = { 0 };
    uint8_t peer_count = 0;
    for (uint32_t i = 0; i < match->peer_count; ++i) {
        const NativeLobbyMatchPeer* source = &match->peers[i];
        if (source == local) {
            continue;
        }
        NativeNetplayRoomPeerLaunch* output = &peers[peer_count++];
        memcpy(output->peer_id, source->peer_id,
               strlen(source->peer_id) + 1);
        output->peer = source->numeric_peer;
        output->port_mask = source->port_mask;
    }

    if(!prepare_match(match))return false;
    memcpy(netplay.room.peers,peers,sizeof(peers));
    netplay.room.peer_count=peer_count;
    launch.netplay=netplay;launch.spectating=false;
    return true;
}

bool nativeMatchLaunch_AdoptSpectatorMatch(const NativeLobbyMatch* match)
{
    if(!match)return false;
    launch.netplay=(NativeNetplayLaunchConfig){0};
    launch.spectating=true;
    return prepare_match(match);
}
bool nativeMatchLaunch_IsSpectating(void)
{
    return launch.spectating;
}

void nativeMatchLaunch_Apply(void)
{
    GameRules* rules = gmMainLib_GetGameRules();
    struct GamePrefs* prefs = gmMainLib_GetGamePrefs();
    VsModeData* vs = gmVsMelee_GetVsData();
    lbDvd_80018C6C();
    PreloadedGameModeState* preload = lbDvd_GetPreloadCacheScene();
    rules->mode = launch.mode;
    rules->time_limit = launch.time_minutes;
    rules->stock_time_limit =
        launch.mode == MatchKind_Stock && !launch.time_explicit
            ? 0
            : launch.time_minutes;
    rules->stock_count = launch.stocks;
    prefs->item_freq = (u8) launch.item_freq;
    prefs->item_mask = launch.item_freq < 0 ? 0 : UINT64_MAX;
    vs->start.rules.stkind = launch.stage;
    vs->start.rules.is_teams = launch.teams;
    preload->game_cache.mode_kind = GM_VS;
    preload->game_cache.stkind = launch.stage;
    for (int i = 0; i < 8; ++i) {
        preload->game_cache.entries[i] =
            (PreloadCacheSceneEntry){ ChKind_None, 0, 1 };
    }
    for (int i = 0; i < 4; ++i) {
        PlayerInitData* player = &vs->start.players[i];
        player->slot_type = launch.slots[i].kind;
        if (player->slot_type != Gm_PKind_NA) {
            player->ckind = launch.slots[i].character;
            player->cpu_kind = 4;
            player->cpu_level = launch.slots[i].cpu_level;
            player->rumble_enabled = false;
            player->team = launch.slots[i].team;
            player->handicap = launch.slots[i].handicap;
            player->color = player->ckind >= ChKind_CustomFirst ? 0 : launch.teams
                                ? team_costume(player->ckind, player->team)
                                : launch.slots[i].color;
            // The fighter bundle is imported after Fighter_FirstInitialize;
            // no original ISO archive exists for a custom character.
            preload->game_cache.entries[i].char_id =
                player->ckind >= ChKind_CustomFirst ? ChKind_None
                                                     : player->ckind;
            preload->game_cache.entries[i].color = player->color;
        }
    }
    if (launch.stage == St_Kind_Practice) {
        preload->game_cache.entries[NATIVE_PRACTICE_SANDBAG_SLOT] =
            (PreloadCacheSceneEntry){ ChKind_Sandbag, 0, 1 };
    }
    lbDvd_80018254();
    lbDvd_80017700(4);
}

bool nativeMatchLaunch_RegisterFighters(void)
{
    if (!launch.direct) {
        return nativeFighterRuntime_ActivateStaged(launch.fighter_count);
    }
    if (launch.direct_prepared) {
        launch.direct_prepared = false;
        return nativeFighterRuntime_ActivateStaged(launch.fighter_count);
    }
    // A later direct-mode scene can still re-enter without startup staging.
    return nativeFighterRuntime_StageCached(launch.fighter_ids,
        launch.fighter_count, launch.content_root,
        !launch.netplay.enabled && !launch.local_preview && !launch.spectating) &&
        nativeFighterRuntime_ActivateStaged(launch.fighter_count);
}

#endif
