#pragma once
#include <string>

struct NativePortraitCamera {
    float yaw = -.35f, pitch = .12f, zoom = 1, pan_x = 0, pan_y = 0, frame = 0;
};
struct NativeSourceResources;

namespace NativeFighterPresentation {
enum class Target { Tile, Card };
struct Framing {
    NativePortraitCamera camera;
    char animation[64]{};
};
struct Image {
    size_t bytes = 0;
    unsigned width = 0, height = 0;
    std::string sha256;
};
struct Snapshot {
    std::string revision, title, tile_name;
    Image images[2];
    Framing framing[2];
};
enum class Result { Saved, Busy, Conflict, Invalid, Failed };
bool read_framing(const NativeSourceResources&, Target, Framing&);
bool inspect(const char* path, Snapshot&, std::string& error);
Result save(const char* path, const char* expected_revision, Target,
            const char* tile_name, const char* image_path, const Framing*,
            std::string& revision, std::string& error);
}
