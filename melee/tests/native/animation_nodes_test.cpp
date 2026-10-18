#undef NDEBUG
#include <melee/ft/native_fighter_animation_nodes.h>
#include <melee/lb/lbanim_dat.h>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>

static unsigned long long checks;
static unsigned trees;
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr, "CHECK %s:%d: %s\n", __FILE__, __LINE__, #x); std::abort(); } } while (0)
static uint32_t read32(const uint8_t* p)
{ return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3]; }
static void word(std::vector<uint8_t>& b, size_t at, uint32_t n)
{ b[at] = n >> 24; b[at+1] = n >> 16; b[at+2] = n >> 8; b[at+3] = n; }

struct Tree {
    FigaTree tree = {};
    FigaTrack tracks[4096];
    u32 count;
    unsigned starts[513] = {};
};
static void decode(const HSD_Dat& dat, u32 root, Tree& t)
{
    CHECK(lbAnim_DecodeFigaTree(&dat, root, &t.tree, t.tracks, 4096, &t.count) == HSD_DAT_OK);
    CHECK(t.count <= 512);
    t.starts[0] = 0;
    for (unsigned i = 0; i < t.count; ++i)
        t.starts[i+1] = t.starts[i] + (uint8_t)t.tree.nodes[i];
}
static void compare(const HSD_Dat& before, const HSD_Dat& after,
                    const std::vector<uint16_t>& order)
{
    CHECK(before.root_count == after.root_count);
    for (u32 i = 0; i < before.root_count; ++i) {
        u32 a, b;
        const char *an, *bn;
        CHECK(HSD_DatSymbol(&before, false, i, &a, &an));
        CHECK(HSD_DatSymbol(&after, false, i, &b, &bn));
        CHECK(!std::strcmp(an, bn));
        CHECK(!std::memcmp(before.data + a, after.data + b, 12));
        Tree src, dst;
        decode(before, a, src);
        decode(after, b, dst);
        CHECK(dst.count == order.size());
        for (unsigned j = 0; j < order.size(); ++j) {
            if (order[j] == UINT16_MAX) { CHECK(dst.tree.nodes[j] == 0); continue; }
            unsigned old = order[j];
            CHECK(src.tree.nodes[old] == dst.tree.nodes[j]);
            for (unsigned k = 0; k < (uint8_t)dst.tree.nodes[j]; ++k) {
                const FigaTrack& x = src.tracks[src.starts[old] + k];
                const FigaTrack& y = dst.tracks[dst.starts[j] + k];
                CHECK(x.length == y.length);
                CHECK(x.startframe == y.startframe);
                CHECK(x.obj_type == y.obj_type);
                CHECK(x.frac_value == y.frac_value);
                CHECK(x.frac_slope == y.frac_slope);
                CHECK(!std::memcmp(x.ad_head, y.ad_head, x.length));
                // The eighth serialized descriptor byte is not exposed by
                // the decoder. Preserve it too, including uncommon encodings.
                u32 ta, tb; const char* external;
                CHECK(HSD_DatResolve(&before, a + 16, &ta, &external) == HSD_DAT_REF_LOCAL);
                CHECK(HSD_DatResolve(&after, b + 16, &tb, &external) == HSD_DAT_REF_LOCAL);
                CHECK(!std::memcmp(before.data + ta + (src.starts[old]+k)*12,
                                   after.data + tb + (dst.starts[j]+k)*12, 8));
            }
        }
        for (u32 k = 0; k < i; ++k) {
            u32 old, now; const char* name;
            CHECK(HSD_DatSymbol(&before, false, k, &old, &name));
            CHECK(HSD_DatSymbol(&after, false, k, &now, &name));
            CHECK((old == a) == (now == b));
        }
    }
}
static std::vector<uint8_t> edit(const void* bytes, size_t size, unsigned old_count,
                                const std::vector<uint16_t>& order)
{
    void* output = nullptr;
    size_t length = 0;
    char error[256];
    bool ok = nativeFighterAnimation_RemapNodes(bytes, size, old_count,
        order.data(), (unsigned)order.size(), &output, &length, error, sizeof(error));
    if (!ok) std::fprintf(stderr, "%s\n", error);
    CHECK(ok && output && length);
    HSD_Dat a, b;
    CHECK(HSD_DatOpen(&a, bytes, size) == HSD_DAT_OK);
    CHECK(HSD_DatOpen(&b, output, length) == HSD_DAT_OK);
    compare(a, b, order);
    std::vector<uint8_t> result((uint8_t*)output, (uint8_t*)output + length);
    std::free(output);
    return result;
}
static void exercise(const void* bytes, size_t size)
{
    HSD_Dat dat;
    CHECK(HSD_DatOpen(&dat, bytes, size) == HSD_DAT_OK);
    u32 root; const char* name;
    CHECK(HSD_DatSymbol(&dat, false, 0, &root, &name));
    Tree tree;
    decode(dat, root, tree);
    CHECK(tree.count > 0);
    std::vector<uint16_t> order;
    for (unsigned i = 0; i < tree.count; ++i) order.push_back((uint16_t)i);
    auto canonical = edit(bytes, size, tree.count, order);
    CHECK(edit(canonical.data(), canonical.size(), tree.count, order) == canonical);
    // Insert an empty bone inside the rig; all following tracks move one node.
    if (tree.count < 512) {
        order.insert(order.begin() + tree.count/2, UINT16_MAX);
        auto inserted = edit(bytes, size, tree.count, order);
        std::vector<uint16_t> restore;
        for (unsigned i = 0; i < tree.count; ++i)
            restore.push_back((uint16_t)(i >= tree.count/2 ? i + 1 : i));
        CHECK(edit(inserted.data(), inserted.size(), tree.count+1, restore) == canonical);
    }
    order.clear();
    for (unsigned i = tree.count; i; --i) order.push_back((uint16_t)(i-1));
    auto reversed = edit(bytes, size, tree.count, order);
    CHECK(edit(reversed.data(), reversed.size(), tree.count, order) == canonical);
    // Removal is explicit; only retained nodes keep their original tracks.
    order.resize(std::max(1u, (unsigned)tree.count/2));
    edit(bytes, size, tree.count, order);
    ++trees;
}
static std::vector<uint8_t> fixture()
{
    // Two independent roots and one alias; both trees share a compressed stream.
    std::vector<uint8_t> b(32 + 100 + 7*4 + 3*8 + 6, 0);
    word(b, 0, (uint32_t)b.size()); word(b, 4, 100); word(b, 8, 7); word(b, 12, 3);
    for (unsigned root : {0u, 20u}) {
        word(b, 32+root, 1); word(b, 32+root+4, 0x10203040);
        word(b, 32+root+8, 0x42440000); // 49 frames.
        word(b, 32+root+12, 40); word(b, 32+root+16, 48);
    }
    b[32+40]=1; b[32+41]=0; b[32+42]=2; b[32+43]=255;
    for (unsigned i = 0; i < 3; ++i) {
        unsigned at = 32+48+i*12;
        b[at+1]=6; b[at+3]=(uint8_t)i;
        b[at+4]=(uint8_t)(i+1); b[at+7]=0x80;
        word(b, at+8, 84);
    }
    b[32+84]=2; b[32+89]=1;
    unsigned at=132;
    for (unsigned slot : {12u,16u,32u,36u,56u,68u,80u}) {word(b,at,slot);at+=4;}
    word(b,at,0);word(b,at+4,0); at+=8;
    word(b,at,20);word(b,at+4,2);at+=8;
    word(b,at,0);word(b,at+4,4);at+=8;
    std::memcpy(b.data()+at, "a\0b\0c\0",6);
    return b;
}
static void rejection(const std::vector<uint8_t>& b, unsigned old_count,
                      std::vector<uint16_t> order)
{
    auto before = b;
    void* out = nullptr; size_t size = 0; char error[256] = {};
    CHECK(!nativeFighterAnimation_RemapNodes(b.data(), b.size(), old_count,
        order.data(), (unsigned)order.size(), &out, &size, error, sizeof(error)));
    CHECK(out == nullptr && size == 0 && *error && before == b);
}
int main(int argc, char** argv)
{
    if (argc == 1) {
        auto b = fixture();
        exercise(b.data(), b.size());
        edit(b.data(), b.size(), 3, std::vector<uint16_t>(512, UINT16_MAX));
        rejection(b, 3, {0,0,1}); rejection(b, 3, {0,1,3});
        rejection(b, 2, {0,1}); rejection(b, 513, {0});
        rejection(b, 3, {});
        rejection(b, 3, std::vector<uint16_t>(513, UINT16_MAX));
        auto broken = b;
        word(broken, 32+80, 99); // A retained track overruns the source data.
        rejection(broken,3,{0,1,2});
        broken = b;
        word(broken, 132+7*4+8+4, 0); // Duplicate root name, distinct tree.
        rejection(broken,3,{0,1,2});
        void* occupied = b.data(); size_t occupied_size = b.size();
        uint16_t identity[] = {0,1,2};
        CHECK(!nativeFighterAnimation_RemapNodes(b.data(),b.size(),3,identity,3,
            &occupied,&occupied_size,nullptr,0));
        CHECK(occupied == b.data() && occupied_size == b.size());
        word(b,32+20+12,99); // Only the second distinct root is malformed.
        rejection(b,3,{0,1,2});
    } else {
        CHECK(argc == 2);
        std::ifstream file(argv[1],std::ios::binary);
        CHECK(file.good());
        std::vector<uint8_t> bank((std::istreambuf_iterator<char>(file)),{});
        for (size_t at=0; at<bank.size();) {
            if (bank[at] == 255) {++at;continue;}
            CHECK(bank.size()-at >= 32);
            unsigned size=read32(bank.data()+at);
            CHECK(size >= 32 && size <= bank.size()-at);
            exercise(bank.data()+at,size);
            at+=size;
        }
    }
    std::printf("PASS %u trees %llu checks\n",trees,checks);
}
