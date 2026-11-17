#include "native_fighter_presentation.h"
#include "native_fighter_draft_access.h"
#include "native_fighter_project.h"
#include "native_gx_image.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>

namespace NativeFighterPresentation {
namespace {
constexpr size_t TileBytes = 1024 * 1024;
constexpr const char* CardImage = "ui/player-card.png";

const char* framing_name(Target target)
{
    return target == Target::Card ? "ui/portrait-card.camera" : "ui/portrait-tile.camera";
}
bool valid_framing(const Framing& framing)
{
    const auto& c = framing.camera;
    return std::isfinite(c.yaw) && std::isfinite(c.pitch) && std::isfinite(c.zoom) &&
        std::isfinite(c.pan_x) && std::isfinite(c.pan_y) && std::isfinite(c.frame) &&
        c.zoom >= .15f && c.zoom <= 32.f && c.frame >= 0 && c.frame <= 1024 &&
        framing.animation[0] && !std::strpbrk(framing.animation, " \t\r\n");
}
bool read_image(const std::filesystem::path& path, size_t limit, std::string& output)
{
    const auto file = std::unique_ptr<FILE, decltype(&std::fclose)>(_wfopen(path.c_str(), L"rb"), std::fclose);
    if (!file || std::fseek(file.get(), 0, SEEK_END)) return false;
    const long size = std::ftell(file.get());
    if (size <= 0 || size_t(size) > limit || std::fseek(file.get(), 0, SEEK_SET)) return false;
    output.resize(size);
    return std::fread(output.data(), 1, output.size(), file.get()) == output.size() &&
           !std::ferror(file.get());
}
bool describe_image(Target target, const void* bytes, size_t size, Image& image, std::string& error)
{
    const auto* png = static_cast<const uint8_t*>(bytes);
    uint16_t width = 0, height = 0;
    char diagnostic[256]{};
    if (target == Target::Tile) {
        if (!nativeContent_ValidFighterPortrait(png, size)) {
            error = "Use a noninterlaced 8-bit RGB or RGBA tile PNG, 16–512 pixels per side and at most 1 MiB.";
            return false;
        }
        width = (uint16_t(png[18]) << 8) | png[19];
        height = (uint16_t(png[22]) << 8) | png[23];
    } else if (!nativeSourceResources_PNG(png, size, &width, &height, diagnostic, sizeof(diagnostic))) {
        error = diagnostic;
        return false;
    }
    unsigned short decoded_width = 0, decoded_height = 0;
    auto* pixels = nativeGxImage_Decode(png, unsigned(size), &decoded_width, &decoded_height);
    const bool valid = pixels && decoded_width == width && decoded_height == height;
    nativeGxImage_Free(pixels);
    NativeContentId digest{};
    if (!valid || !nativeContent_Hash(bytes, size, &digest)) {
        error = "Could not decode the complete portrait PNG.";
        return false;
    }
    char hex[65];
    nativeContentStore_IdHex(&digest, hex);
    image = {size, width, height, hex};
    return true;
}
struct Project {
    NativeFighterProject value{};
    ~Project() { nativeFighterProject_Close(&value); }
};
}

bool read_framing(const NativeSourceResources& resources, Target target, Framing& framing)
{
    const auto* record = nativeSourceResources_Find(&resources, framing_name(target), NativeSourceResource_Blob);
    char text[192]{};
    if (!record || record->size >= sizeof(text)) return false;
    std::memcpy(text, record->bytes, record->size);
    Framing decoded;
    auto& c = decoded.camera;
    if (std::sscanf(text, "%f %f %f %f %f %f %63s", &c.yaw, &c.pitch, &c.zoom,
                    &c.pan_x, &c.pan_y, &c.frame, decoded.animation) != 7 ||
        !valid_framing(decoded)) return false;
    framing = decoded;
    return true;
}

bool inspect(const char* path, Snapshot& snapshot, std::string& error)
{
    snapshot = {};
    if (!NativeDraftAccess::revision(path, snapshot.revision, error)) return false;
    Project project;
    NativeSourceResources resources{};
    if (!nativeFighterProject_Open(path, &project.value) ||
        !nativeSourceResources_Read(project.value.resources, project.value.resource_size, &resources, nullptr, 0)) {
        error = "Could not read the fighter presentation from this draft.";
        return false;
    }
    snapshot.title = project.value.title;
    snapshot.tile_name = project.value.tile_name;
    const auto tile_path = std::filesystem::u8path(path) / "portrait.png";
    std::error_code status;
    if (std::filesystem::exists(tile_path, status)) {
        std::string png;
        if (!read_image(tile_path, TileBytes, png)) {
            error = "Could not read the tile PNG within its 1 MiB size limit.";
            return false;
        }
        if (!describe_image(Target::Tile, png.data(), png.size(), snapshot.images[0], error)) return false;
    } else if (status) {
        error = "Could not inspect the tile image.";
        return false;
    }
    const auto* card = nativeSourceResources_Find(&resources, CardImage, NativeSourceResource_PNG);
    if (card && !describe_image(Target::Card, card->bytes, card->size, snapshot.images[1], error)) return false;
    read_framing(resources, Target::Tile, snapshot.framing[0]);
    read_framing(resources, Target::Card, snapshot.framing[1]);
    std::string after;
    if (!NativeDraftAccess::revision(path, after, error)) return false;
    if (after == snapshot.revision) return true;
    error = "Draft changed while reading the portrait. Inspect it again.";
    return false;
}

Result save(const char* path, const char* expected_revision, Target target,
            const char* tile_name, const char* image_path, const Framing* framing,
            std::string& after, std::string& error)
{
    NativeDraftAccess::Lease lease;
    if (!lease.acquire(path, error)) return Result::Busy;
    std::string before;
    if (!NativeDraftAccess::revision(path, before, error)) return Result::Failed;
    if (expected_revision && before != expected_revision) {
        error = "The draft changed. Reopen the portrait or inspect the saved draft before editing.";
        return Result::Conflict;
    }
    std::vector<NativeDraftAccess::FileEdit> edits;
    if (tile_name) {
        NativeFighterProjectInfo info{};
        if (!nativeFighterProject_ReadInfo(path, &info)) {
            error = "Could not read the fighter manifest.";
            return Result::Failed;
        }
        char manifest[256];
        const size_t size = nativeFighterProject_EncodeManifest(info.title, tile_name, info.template_key,
                                                                manifest, sizeof(manifest));
        if (!size) {
            error = "Enter a tile name of at most 63 bytes without control characters.";
            return Result::Invalid;
        }
        if (std::strcmp(info.tile_name, tile_name)) edits.push_back({"project.txt", std::string(manifest, size)});
    }
    std::string png;
    if (image_path) {
        if (!read_image(std::filesystem::u8path(image_path),
                        target == Target::Tile ? TileBytes : size_t(NATIVE_SOURCE_RESOURCE_PNG_BYTES), png)) {
            error = "Choose a readable PNG within the target's file size limit.";
            return Result::Invalid;
        }
        Image image;
        if (!describe_image(target, png.data(), png.size(), image, error)) return Result::Invalid;
        if (target == Target::Tile) edits.push_back({"portrait.png", std::move(png)});
    }
    if (framing || (image_path && target == Target::Card)) {
        Project project;
        if (!nativeFighterProject_Open(path, &project.value)) {
            error = "Could not open the draft resource bundle.";
            return Result::Failed;
        }
        if (image_path && target == Target::Card &&
            !nativeFighterProject_EditResource(&project.value, CardImage, NativeSourceResource_PNG,
                                               png.data(), png.size())) {
            error = "Could not add the player card to the resource bundle.";
            return Result::Invalid;
        }
        if (framing) {
            const auto& c = framing->camera;
            char text[192];
            int size = std::snprintf(text, sizeof(text), "%.9g %.9g %.9g %.9g %.9g %.9g %s\n",
                c.yaw, c.pitch, c.zoom, c.pan_x, c.pan_y, c.frame, framing->animation);
            if (!valid_framing(*framing) || size <= 0 || size >= sizeof(text) ||
                !nativeFighterProject_EditResource(&project.value, framing_name(target), NativeSourceResource_Blob, text, size)) {
                error = "Could not save the portrait framing to the resource bundle.";
                return Result::Invalid;
            }
        }
        edits.push_back({"resources.fmr", std::string(reinterpret_cast<const char*>(project.value.resources),
                                                       project.value.resource_size)});
    }
    if (edits.empty()) {
        after = before;
        return Result::Saved;
    }
    return NativeDraftAccess::apply(path, before, edits, after, error) ? Result::Saved : Result::Failed;
}
}
