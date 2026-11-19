// Exercise the production AX driver, including its track-wide stop path.
// The synth boundary counts stops; no audio device or game window is opened.
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "audio_rollback_driver.inc"

static unsigned stopped;
static u8 last_pan, last_volume;
bool HSD_SynthHasPackageSound(int id) { return id == HSD_PACKAGE_SOUND_BASE; }
void HSD_SynthSFXKeyOff(int instance) { (void)instance; ++stopped; }
void HSD_SynthSFXSetUserVol(int instance, u8 pan) { (void)instance; last_pan = pan; }
void HSD_SynthSFXSetVolumeFade(int instance, u8 volume, int frames)
{ (void)instance; (void)frames; last_volume = volume; }
void OSReport(char* format, ...) { (void)format; }
void __assert(char* file, u32 line, char* expression)
{ fprintf(stderr, "%s:%lu: %s\n", file, line, expression); abort(); }

static void require(bool condition, const char* message)
{ if (!condition) { fprintf(stderr, "%s\n", message); exit(1); } }

static int start_voice(void)
{
    static u32 words[1], banks[1], streams[1];
    sfx_commands = (HSD_SfxCommands){words, banks, streams, 1, 1};
    memset(AXDriver_804C45A0, 0, sizeof(AXDriver_804C45A0));
    memset(AXDriver_804C5920, 0, sizeof(AXDriver_804C5920));
    for (unsigned i = 0; i < 0x60; ++i) {
        AXDriver_804C45A0[i].prev = i ? &AXDriver_804C45A0[i-1] : NULL;
        AXDriver_804C45A0[i].next = i + 1 < 0x60 ? &AXDriver_804C45A0[i+1] : NULL;
    }
    AXDriver_804D7790 = AXDriver_804C45A0; AXDriver_804D7794 = NULL;
    AXDriver_804D77CC = AXDriver_804D77C8 = AXDriver_804D77D0 = 0;
    vidhigh = 1; stopped = 0;
    HSD_AudioRollbackReset();
    require(HSD_AudioRollbackBeginFrame(99, false), "begin earlier frame");
    require(HSD_AudioRollbackEndFrame(), "end earlier frame");
    require(HSD_AudioRollbackBeginFrame(100, false), "begin predicted voice frame");
    int handle = AXDriver_8038CFF4(0, 127, 64, 30, 7);
    require(handle >= 0, "start predicted voice");
    HSD_SM* voice = &AXDriver_804C45A0[handle & 127];
    voice->vID = 1; AXDriver_804C5920[1] = voice; ++AXDriver_804D77C8;
    require(HSD_AudioRollbackEndFrame(), "end predicted voice frame");
    return handle;
}

int main(void)
{
    int handle = start_voice();
    require(HSD_AudioRollbackBeginFrame(99, true), "replay earlier frame");
    HSD_AudioSFXKeyOffTrack(30);
    require(stopped == 0, "Earlier replayed track stop cut off the future predicted voice");
    AXDriverKeyOff(handle);
    HSD_AudioSFXKeyOffAll();
    require(stopped == 0, "Earlier replayed single/all stop cut off the future voice");
    require(HSD_AudioRollbackEndFrame(), "finish earlier replay frame");
    require(HSD_AudioRollbackBeginFrame(100, true), "replay voice frame");
    HSD_AudioSFXKeyOffTrack(30);
    require(stopped == 0, "Same-frame stop before start cut off its reused voice");
    require(AXDriver_8038CFF4(0, 127, 64, 30, 7) == handle, "reuse original handle");
    require(HSD_AudioRollbackEndFrame(), "finish reused voice frame");
    require(stopped == 0 && (AXDriver_804C45A0[handle & 127].flags & SMSTATE_MASK),
            "Reused vocalization must remain live");

    // A real stop after the start remains effective, including in resimulation.
    require(HSD_AudioRollbackBeginFrame(101, false), "record next frame");
    require(HSD_AudioRollbackEndFrame(), "finish next frame");
    require(HSD_AudioRollbackBeginFrame(101, true), "replay real interruption");
    HSD_AudioSFXKeyOffTrack(30);
    require(stopped == 1, "Corrected interruption must still stop a prior voice");
    require(HSD_AudioRollbackEndFrame(), "finish interruption");

    handle = start_voice();
    require(HSD_AudioRollbackBeginFrame(100, true), "replay canceled sound");
    require(HSD_AudioRollbackEndFrame(), "cancel sound absent from corrected frame");
    require(stopped == 1, "Mispredicted sound must still be canceled");

    handle = start_voice();
    require(HSD_AudioRollbackBeginFrame(100, true), "replay same-frame interruption");
    require(AXDriver_8038CFF4(0, 127, 64, 30, 7) == handle, "replay start before stop");
    HSD_AudioSFXKeyOffAll();
    require(stopped == 1, "Stop after a replayed same-frame start remains effective");
    require(HSD_AudioRollbackEndFrame(), "finish same-frame interruption");

    handle = start_voice();
    require(HSD_AudioRollbackBeginFrame(100, true), "replay changed spatial mix");
    require(AXDriver_8038CFF4(0, 91, 112, 30, 7) == handle,
            "Volume/pan corrections must reuse the same voice without restarting it");
    require(last_pan == 112 && last_volume == 91, "Apply the corrected spatial mix");
    require(HSD_AudioRollbackEndFrame() && stopped == 0,
            "A spatial mix correction must not cancel the utterance");
    require(HSD_AudioRollbackBeginFrame(100, true), "repeat correction");
    HSD_AudioSFXKeyOffTrack(30);
    require(stopped == 0, "Repeated correction still protects unreached voice");
    require(AXDriver_8038CFF4(0, 127, 64, 30, 7) == handle, "repeated reuse");
    require(HSD_AudioRollbackEndFrame() && stopped == 0, "repeat preserves voice");
    require(HSD_AudioRollbackGetStats().protected_stops == 1, "Count protected stop");

    handle = start_voice();
    require(HSD_AudioRollbackBeginFrame(99, true), "replay new corrected sound");
    int added = AXDriver_8038CFF4(0, 127, 64, 31, 7);
    require(added >= 0 && added != handle, "allocate a new corrected voice");
    require(AXDriverKeyOff(added), "New corrected start can stop in its own frame");
    require(stopped == 0, "Stopping the new voice preserves the future synth voice");
    require(HSD_AudioRollbackEndFrame(), "finish new corrected sound");

    handle = start_voice();
    // Reuse of a physical slot must never inherit another generation's guard.
    AXDriver_804C45A0[handle & 127].unk = handle + 128;
    require(HSD_AudioRollbackBeginFrame(99, true), "replay reused pool slot");
    require(AXDriverKeyOff(handle + 128) && stopped == 1,
            "Untracked pool generation remains stoppable");
    require(HSD_AudioRollbackEndFrame(), "finish reused pool slot");

    handle = start_voice();
    require(HSD_AudioRollbackBeginFrame(101, false), "ordinary forward frame");
    require(AXDriverKeyOff(handle) && stopped == 1, "Forward-play stops remain immediate");
    require(HSD_AudioRollbackEndFrame(), "finish forward frame");

    start_voice();
    require(HSD_AudioRollbackBeginFrame(101, false), "record package audio");
    int package = AXDriver_8038CFF4(HSD_PACKAGE_SOUND_BASE, 127, 64, 0, 7);
    require(package >= 0, "start package sample");
    HSD_SM* sample = &AXDriver_804C45A0[package & 127];
    require(sample->cmd_stream[0] == (0x01000000 | HSD_PACKAGE_SOUND_BASE) &&
            sample->cmd_stream[1] == 0x0E000000,
            "play the exact package sample and wait for completion");
    require(HSD_AudioRollbackEndFrame(), "end package audio frame");
    require(HSD_AudioRollbackBeginFrame(101, true), "replay package audio");
    require(AXDriver_8038CFF4(HSD_PACKAGE_SOUND_BASE, 127, 64, 0, 7) == package,
            "rollback reuses a package sample handle");
    require(HSD_AudioRollbackEndFrame(), "end replayed package audio frame");
    AXDriverRetirePackageSounds(HSD_PACKAGE_SOUND_BASE, 1);
    require(!(sample->flags & SMSTATE_MASK) && stopped == 0,
            "discard pending package sample without stopping stock voices");
    puts("Production driver preserves replayed voices and honors corrected stops");
    return 0;
}
