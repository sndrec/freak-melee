#include <sysdolphin/baselib/sfxbank.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void check(int ok, const char* message)
{
    if (!ok) { fprintf(stderr, "%s\n", message); exit(1); }
}
static u32 be32(const u8* p)
{
    return ((u32) p[0] << 24) | ((u32) p[1] << 16) |
           ((u32) p[2] << 8) | p[3];
}
static u32 joined(u16 hi, u16 lo) { return ((u32) hi << 16) | lo; }

static void check_package_bank(void)
{
    u8 bytes[128] = {
        [3] = 72, [7] = 32, [11] = 1,
        [19] = 1, [22] = 0x7D, // One channel at 32000 Hz.
        [35] = 15, [39] = 2,
    };
    HSD_SfxBank* bank = HSD_SfxBankReadPackage(bytes, sizeof(bytes), 34);
    check(bank && bank->count == 1, "read standalone sound bank");
    const HSD_SfxEntry* sample = &bank->entries[0];
    check(sample->id == HSD_PACKAGE_SOUND_BASE + 34 * HSD_PACKAGE_SOUND_CLIPS &&
          bank->sample_data == bytes + 96 && bank->sample_bytes == 32,
          "assign owner sound ID and borrow exact sample data");
    free(bank);
    check(!HSD_SfxBankReadPackage(bytes, sizeof(bytes) - 1, 34),
          "reject truncated sound bank samples");
    bytes[35] = 66;
    check(!HSD_SfxBankReadPackage(bytes, sizeof(bytes), 34),
          "reject out-of-range DSP sample end");
    bytes[35] = 31;
    bytes[104] = 0x80;
    check(!HSD_SfxBankReadPackage(bytes, sizeof(bytes), 34),
          "reject an invalid predictor in the referenced second block");
    bytes[35] = 15;
    bank = HSD_SfxBankReadPackage(bytes, sizeof(bytes), 34);
    check(bank != NULL, "ignore padding outside the voice's sample range");
    free(bank);
    bytes[39] = 1;
    check(!HSD_SfxBankReadPackage(bytes, sizeof(bytes), 34),
          "reject a start within the DSP predictor header");
}

int main(int argc, char** argv)
{
    check_package_bank();
    check(argc == 2, "expected extracted bank headers");
    FILE* input = fopen(argv[1], "rb");
    check(input != NULL, "open fixtures");
    u8 header[16];
    unsigned banks = 0, mono = 0, stereo = 0;
    while (fread(header, 1, 16, input) == 16) {
        size_t size = be32(header);
        u32 count = be32(header + 8);
        u8* data = malloc(size);
        HSD_SfxBank* bank = calloc(1, sizeof(*bank) + count * sizeof(HSD_SfxEntry));
        check(data && bank && fread(data, 1, size, input) == size, "bank fixture read");
        bank->count = count;
        bank->first_id = be32(header + 12);
        check((uintptr_t) bank > 0xFFFFFFFFULL, "native bank address above 4 GiB");
        check(HSD_SfxBankDecode(bank, data, size, 0x123400, 0x8000), "decode original bank");
        // Move down across multiple address high-word boundaries, as compaction does.
        HSD_SfxBankRelocate(bank, 0x45600, 0x8000);
        size_t cursor = 0;
        for (u32 i = 0; i < count; ++i) {
            HSD_SfxEntry* e = &bank->entries[i];
            check(e->id == bank->first_id + i && e->next == NULL, "entry identity");
            check(e->voice_count == be32(data + cursor) &&
                  e->sample_rate == be32(data + cursor + 4), "entry metadata");
            mono += e->voice_count == 1;
            stereo += e->voice_count == 2;
            cursor += 8;
            for (u32 j = 0; j < e->voice_count; ++j) {
                HSD_SfxVoice* v = &e->voices[j];
                u32 loop = v->address.loopFlag ? be32(data + cursor + 4) + 0x8AC00 : 0x8000;
                check(joined(v->address.loopAddressHi, v->address.loopAddressLo) == loop, "loop relocation");
                check(joined(v->address.endAddressHi, v->address.endAddressLo) == be32(data + cursor + 8) + 0x8AC00, "end relocation");
                check(joined(v->address.currentAddressHi, v->address.currentAddressLo) == be32(data + cursor + 12) + 0x8AC00, "current relocation");
                u16 words[23];
                memcpy(words, &v->adpcm, 40);
                memcpy(words + 20, &v->loop, 6);
                for (u32 k = 0; k < 23; ++k) {
                    check(words[k] == ((u16) data[cursor + 16 + k * 2] << 8 |
                                      data[cursor + 17 + k * 2]), "coefficient/history byte order");
                }
                cursor += 64;
            }
        }
        check(cursor == size, "full header consumption");
        check(!HSD_SfxBankDecode(bank, data, size - 1, 0, 0), "reject truncation");
        data[3] = 3;
        check(!HSD_SfxBankDecode(bank, data, size, 0, 0), "reject excess voices");
        free(data); free(bank); ++banks;
    }
    fclose(input);
    check(banks == 110 && mono == 2795 && stereo == 290, "original bank coverage");
    printf("Verified %u banks, %u mono and %u stereo entries\n", banks, mono, stereo);
    return 0;
}
