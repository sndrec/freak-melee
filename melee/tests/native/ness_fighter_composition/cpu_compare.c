// Actual generated assembled callbacks versus unchanged original CPU bodies.
// Shared command byte writes are original C, including the fixed buffer bound.
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/types.h>
#include <melee/ft/ftcmdscript.h>
#include <melee/ft/kinds/ftNess/forward.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <sysdolphin/baselib/gobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static unsigned checks, pairs;
static unsigned candidate_pairs, threshold_deltas, random_calls;
static unsigned threshold_cases[5];
static double maximum_threshold_distance;
static Fighter fighter, victim;
static HSD_GObj source;
static const NativeFighterDefinition* definition;

#define CHECK(condition) do { ++checks; if (!(condition)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); exit(1); \
} } while (0)
#undef HSD_ASSERTREPORT
#define HSD_ASSERTREPORT(line, condition, message) CHECK(condition)
#include "cpu_original.h"
#include "random_original.h"
static float counted_random(void) { ++random_calls; return HSD_Randf(); }
#define HSD_Randf counted_random
#include "candidate_original.h"
#undef HSD_Randf
static void stock_slot(HSD_GObj* gobj) { (void) gobj; }
#include "base.h"
#include "behavior.generated.c"

static void unexpected(void) { CHECK(false); }
static bool alive(HSD_GObj* object, uint32_t generation, uint32_t kind)
{
    return object == &source && source.user_data == &fighter &&
           generation == (uint32_t)fighter.x8_spawnNum &&
           kind == NativeCallbackSource_Fighter;
}
typedef struct Result {
    unsigned count;
    unsigned char bytes[256];
    int behavior;
    int travel_frames;
    bool has_target;
} Result;
static Result result(void)
{
    Result r = {0};
    r.count = (unsigned)(fighter.cpu.write_pos - fighter.cpu.buffer);
    CHECK(r.count <= sizeof(r.bytes));
    memcpy(r.bytes, fighter.cpu.buffer, sizeof(r.bytes));
    r.behavior = fighter.cpu.x18;
    r.travel_frames = fighter.cpu.x90;
    r.has_target = fighter.cpu.x44 != NULL;
    return r;
}
static void reset(int motion, int level, bool target, float dy, unsigned offset)
{
    memset(&fighter, 0, sizeof(fighter));
    memset(&victim, 0, sizeof(victim));
    source = (HSD_GObj){.user_data = &fighter, .classifier = HSD_GOBJ_CLASS_FIGHTER};
    fighter.gobj = &source;
    fighter.kind = Ft_Kind_Ness;
    fighter.x8_spawnNum = 0x80000099u;
    fighter.motion_id = motion;
    fighter.cpu.level = level;
    fighter.cpu.x18 = 4;
    fighter.cpu.x1C = 7;
    fighter.cur_pos = (Vec3){5, 20, 1};
    victim.cur_pos = (Vec3){6, 20 + dy, 1};
    fighter.cpu.x44 = target ? &victim : NULL;
    memset(fighter.cpu.buffer, 0xA5, sizeof(fighter.cpu.buffer));
    CHECK(offset <= sizeof(fighter.cpu.buffer));
    fighter.cpu.write_pos = fighter.cpu.buffer + offset;
}
static void load(const char* path)
{
    FILE* file = fopen(path, "rb"); CHECK(file != NULL);
    char text[NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY + 1];
    size_t length = fread(text, 1, sizeof(text)-1, file);
    CHECK(!ferror(file) && feof(file)); fclose(file);
    NativeFighterParameters params;
    CHECK(nativeFighterParameters_Parse(text, length, &params, NULL));
    uint32_t words[NATIVE_FIGHTER_PARAMETER_CAPACITY];
    for (unsigned i = 0; i < params.count; ++i) words[i] = params.fields[i].bits;
    CHECK(FreakMelee_SetFighterParameters(words, params.count, params.schema_hash));
}
static void compare_active(void)
{
    static const float heights[] = {-25.0001f,-25,-24.9999f,0,24.9999f,25,25.0001f};
    for (int motion = 347; motion <= 367; ++motion)
        for (unsigned target = 0; target < 2; ++target)
            for (unsigned h = 0; h < sizeof(heights)/sizeof(*heights); ++h) {
                bool handled = (motion >=348 && motion <=351) ||
                               (motion >=358 && motion <=366);
                reset(motion, 9, target, heights[h], 0);
                if (motion >=348 && motion <=351) ftCo_800B920C(&fighter);
                else if (motion >=358 && motion <=366) ftCo_800B92D4(&fighter);
                Result expected = result();
                // Test empty, prefixed and exactly-fitting command buffers.
                unsigned starts[] = {0,1,256-expected.count};
                for (unsigned start = 0; start < 3; ++start) {
                    reset(motion, 9, target, heights[h], starts[start]);
                    if (motion >=348 && motion <=351) ftCo_800B920C(&fighter);
                    else if (motion >=358 && motion <=366) ftCo_800B92D4(&fighter);
                    expected = result();
                    for (unsigned callback = 0; callback < 2; ++callback) {
                        reset(motion, 9, target, heights[h], starts[start]);
                        bool actual = callback ? definition->on_cpu_travel_decision(&source) :
                                                 definition->on_cpu_attack_decision(&source);
                        Result now = result();
                        // The original ftCo_800B9020 Ness branch increments
                        // x90 after its special policy returns. The authored
                        // travel callback now owns that exact update.
                        Result callback_expected = expected;
                        if (callback && handled) ++callback_expected.travel_frames;
                        CHECK(actual == handled);
                        CHECK(!memcmp(&callback_expected, &now, sizeof(now))); ++pairs;
                    }
                }
            }
}
static void compare_recovery(void)
{
    static const float xs[] = {-1,0,1};
    static const float ys[] = {29.9999f,30,30.0001f};
    for (int level = 1; level <= 9; ++level)
        for (unsigned x = 0; x < 3; ++x)
            for (unsigned y = 0; y < 3; ++y) {
                reset(360, level, false, 0, 0);
                fighter.cpu.x54 = (Vec2){5+xs[x],ys[y]};
                if (fighter.cur_pos.y > fighter.cpu.x54.y-10)
                    ftCo_800B463C(&fighter, CpuCmd_Done);
                else ftCo_800A8EB0(&fighter);
                Result expected = result();
                unsigned starts[] = {0,1,256-expected.count};
                for (unsigned start = 0; start < 3; ++start) {
                    reset(360, level, false, 0, starts[start]);
                    fighter.cpu.x54 = (Vec2){5+xs[x],ys[y]};
                    if (fighter.cur_pos.y > fighter.cpu.x54.y-10)
                        ftCo_800B463C(&fighter, CpuCmd_Done);
                    else ftCo_800A8EB0(&fighter);
                    expected = result();
                    reset(360, level, false, 0, starts[start]);
                    fighter.cpu.x54 = (Vec2){5+xs[x],ys[y]};
                    definition->on_cpu_recovery(&source);
                    Result now = result();
                    CHECK(!memcmp(&expected, &now, sizeof(now))); ++pairs;
                }
            }
}
static void compare_candidate(void)
{
    const double thresholds[]={-.8726646192371845,-.6981316953897476,
        -.08726646192371845,.08726646192371845,1.0471975430846214};
    float angles[18]={-2,0,2};
    for(unsigned t=0;t<5;++t) {
        angles[3+t*3]=nextafterf((float)thresholds[t],-INFINITY);
        angles[4+t*3]=(float)thresholds[t];
        angles[5+t*3]=nextafterf((float)thresholds[t],INFINITY);
    }
    const float dx[]={-2,-.9999f,0,.9999f,2}, dy[]={-1,0,1}, distances[]={52.999f,53,53.001f};
    const uint32_t seeds[]={1,77,0x80000001u,0xFFFFFFFFu};
    for(unsigned air=0;air<2;++air) for(unsigned facing=0;facing<2;++facing)
      for(unsigned x=0;x<5;++x) for(unsigned y=0;y<3;++y)
      for(unsigned d=0;d<3;++d) for(unsigned a=0;a<18;++a)
      for(unsigned r=0;r<4;++r) for(unsigned selected=0;selected<2;++selected) {
        reset(14,9,true,dy[y],0); fighter.ground_or_air=air?GA_Air:GA_Ground;
        fighter.facing_dir=facing?-1:1; victim.cur_pos.x=fighter.cur_pos.x+dx[x];
        fighter.cpu.xA4=selected?31:0; seed=seeds[r]; random_calls=0;
        bool expected=ftCo_800B9790(&fighter,angles[a],distances[d]);
        int command=fighter.cpu.xA4; uint32_t final_seed=seed; unsigned calls=random_calls;
        reset(14,9,true,dy[y],0); fighter.ground_or_air=air?GA_Air:GA_Ground;
        fighter.facing_dir=facing?-1:1; victim.cur_pos.x=fighter.cur_pos.x+dx[x];
        fighter.cpu.xA4=selected?31:0; seed=seeds[r]; random_calls=0;
        bool actual=definition->on_cpu_special_candidate(&source,angles[a],distances[d]);
        if(expected!=actual || command!=fighter.cpu.xA4 || final_seed!=seed || calls!=random_calls) {
            // FMP parameters are binary32. Only equality at one rounded original
            // double threshold may alter the strict comparison, not a range of
            // angles or unrelated decision state. Its two neighboring Floats
            // are tested above and must remain exact.
            int edge=-1;
            for(unsigned t=0;t<5;++t) if(angles[a]==(float)thresholds[t]) {
                double delta=fabs((double)angles[a]-thresholds[t]);
                CHECK(delta<=fabs((double)nextafterf(angles[a],INFINITY)-angles[a]));
                if(delta>maximum_threshold_distance) maximum_threshold_distance=delta;
                edge=(int)t;
            }
            CHECK(edge>=0);
            if(edge==4) {
                // This single rounded Float is above the original double but
                // equal to the authored threshold: high Flash becomes the
                // ordinary one-draw low-Flash/retained-command branch.
                CHECK(!air && dy[y]>0 && expected && actual && command==20);
                CHECK(calls==0 && random_calls==1);
                CHECK(seed==seeds[r]*UINT32_C(214013)+UINT32_C(2531011));
                CHECK(fighter.cpu.xA4==31 || fighter.cpu.xA4==(selected?31:0));
            } else {
                CHECK(a>=3 && a<=14 && ((a-4)%3)==0 && distances[d]<53);
                CHECK(command==27 || fighter.cpu.xA4==27);
                CHECK(expected || actual);
            }
            ++threshold_cases[edge];
            ++threshold_deltas;
        } else {
            CHECK(expected==actual && command==fighter.cpu.xA4);
            CHECK(final_seed==seed && calls==random_calls);
        }
        ++candidate_pairs;
      }
}
int main(int argc, char** argv)
{
    CHECK(argc == 2);
    NativeFighterHost host = {0};
#include "required_host.h"
    host.callback_source_alive = alive;
    host.cpu_command = ftCo_800B463C;
    host.cpu_command_arg = ftCo_800B46B8;
    host.cpu_command_pair = ftCo_800B4778;
    host.sin_float = sinf;
    host.cos_float = cosf;
    host.abs_float = fabsf;
    host.random_float = counted_random;
    definition = FreakMelee_GetFighterModule(&ftNessClone_Definition, &host);
    CHECK(definition != NULL);
    load(argv[1]);
    compare_active(); compare_recovery(); compare_candidate();
    CHECK(candidate_pairs==25920 && threshold_deltas==624);
    CHECK(threshold_cases[0]==0 && threshold_cases[1]==0 &&
          threshold_cases[2]==192 && threshold_cases[3]==192 && threshold_cases[4]==240);
    printf("Ness composed CPU: %u checks, 0 failures, %u source pairs\n", checks, pairs);
    printf("Ness candidate CPU: %u pairs, %u bounded threshold deltas, maximum angle delta %.17g\n",
           candidate_pairs,threshold_deltas,maximum_threshold_distance);
    printf("Threshold delta counts: %u %u %u %u %u\n",threshold_cases[0],threshold_cases[1],
           threshold_cases[2],threshold_cases[3],threshold_cases[4]);
    return 0;
}
