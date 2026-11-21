#include <wincodec.h>
#include <wrl/client.h>
#include <malloc.h>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>
#include "native_source_resources.h"

#include "native_gx_image.h"

// WIC only needs the pixel data, palette, transparency and color declarations.
// Skip embedded text and profiles without passing them to a metadata decoder.
static bool prepare_png(const BYTE*& bytes, unsigned& size, std::unique_ptr<BYTE[]>& copy)
{
    uint16_t width, height;
    if (!nativePng_Inspect(bytes, size, 8 * 1024 * 1024, &width, &height)) return false;
    unsigned kept = 8;
    for (unsigned at = 8; at < size; ) {
        const BYTE* chunk = bytes + at;
        unsigned length = (unsigned(chunk[0]) << 24) | (unsigned(chunk[1]) << 16) |
                          (unsigned(chunk[2]) << 8) | chunk[3];
        const unsigned step = 12 + length;
        const BYTE* name = chunk + 4;
        if (!(name[0] & 0x20) || !memcmp(name, "tRNS", 4) ||
            (length == 1 && !memcmp(name, "sRGB", 4)) ||
            (length == 4 && !memcmp(name, "gAMA", 4)) ||
            (length == 32 && !memcmp(name, "cHRM", 4))) {
            if (copy) memcpy(copy.get() + kept, chunk, step);
            kept += step;
        } else if (!copy) {
            copy.reset(new(std::nothrow) BYTE[size]);
            if (!copy) return false;
            memcpy(copy.get(), bytes, kept);
        }
        at += step;
    }
    if (copy) bytes = copy.get();
    size = kept;
    return true;
}

extern "C" unsigned char* nativeImage_DecodeRGBA(
    const unsigned char* encoded, unsigned size, unsigned short* out_width,
    unsigned short* out_height)
{
    using Microsoft::WRL::ComPtr;
    if (!encoded || size < 3 || size > 8 * 1024 * 1024 || !out_width || !out_height)
        return nullptr;
    const bool jpeg = encoded[0] == 0xff && encoded[1] == 0xd8 && encoded[2] == 0xff;
    std::unique_ptr<BYTE[]> filtered;
    if (!jpeg && !prepare_png(encoded, size, filtered)) return nullptr;
    const HRESULT initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(initialized) && initialized != RPC_E_CHANGED_MODE) return nullptr;
    struct Apartment {
        HRESULT result;
        ~Apartment() {
            if (result == S_OK || result == S_FALSE) CoUninitialize();
        }
    } apartment{initialized};
    ComPtr<IWICImagingFactory> factory;
    ComPtr<IWICStream> stream;
    ComPtr<IWICBitmapDecoder> decoder;
    ComPtr<IWICBitmapFrameDecode> frame;
    ComPtr<IWICFormatConverter> converter;
    UINT width = 0, height = 0;
    const bool ready =
        SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                                   CLSCTX_INPROC_SERVER,
                                   IID_PPV_ARGS(&factory))) &&
        SUCCEEDED(factory->CreateStream(&stream)) &&
        SUCCEEDED(stream->InitializeFromMemory(const_cast<BYTE*>(encoded), size)) &&
        SUCCEEDED(CoCreateInstance(jpeg ? CLSID_WICJpegDecoder : CLSID_WICPngDecoder,
                                   nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&decoder))) &&
        SUCCEEDED(decoder->Initialize(stream.Get(), WICDecodeMetadataCacheOnDemand)) &&
        SUCCEEDED(decoder->GetFrame(0, &frame)) &&
        SUCCEEDED(frame->GetSize(&width, &height)) &&
        width > 0 && width <= 1024 && height > 0 && height <= 1024 &&
        SUCCEEDED(factory->CreateFormatConverter(&converter)) &&
        SUCCEEDED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA,
                                        WICBitmapDitherTypeNone, nullptr, 0,
                                        WICBitmapPaletteTypeCustom));
    if (!ready) return nullptr;
    const UINT pixel_bytes = width * height * 4;
    auto* rgba = static_cast<unsigned char*>(malloc(pixel_bytes));
    if (!rgba) return nullptr;
    if (FAILED(converter->CopyPixels(nullptr, width * 4, pixel_bytes, rgba))) {
        free(rgba);
        return nullptr;
    }
    *out_width = static_cast<unsigned short>(width);
    *out_height = static_cast<unsigned short>(height);
    return rgba;
}

extern "C" unsigned char* nativeGxImage_Decode(
    const unsigned char* encoded, unsigned size, unsigned short* out_width,
    unsigned short* out_height)
{
    if (!out_width || !out_height) return nullptr;
    unsigned short width, height;
    auto* rgba = nativeImage_DecodeRGBA(encoded, size, &width, &height);
    if (!rgba) return nullptr;
    const unsigned padded_width = (width + 3) & ~3u;
    const unsigned padded_height = (height + 3) & ~3u;
    auto* tiled = static_cast<unsigned char*>(
        _aligned_malloc(size_t(padded_width) * padded_height * 4, 32));
    if (tiled) {
        for (unsigned by = 0; by < padded_height; by += 4)
            for (unsigned bx = 0; bx < padded_width; bx += 4) {
                unsigned char* tile = tiled +
                    (by / 4 * (padded_width / 4) + bx / 4) * 64;
                for (unsigned y = 0; y < 4; ++y)
                    for (unsigned x = 0; x < 4; ++x) {
                        const unsigned index = y * 4 + x;
                        const unsigned sx = bx + x, sy = by + y;
                        const unsigned char* p = rgba +
                            (size_t(sy < height ? sy : height - 1) * width +
                             (sx < width ? sx : width - 1)) * 4;
                        tile[index * 2] = p[3];
                        tile[index * 2 + 1] = p[0];
                        tile[32 + index * 2] = p[1];
                        tile[32 + index * 2 + 1] = p[2];
                    }
            }
        *out_width = width;
        *out_height = height;
    }
    free(rgba);
    return tiled;
}

extern "C" void nativeGxImage_Free(unsigned char* pixels)
{
    _aligned_free(pixels);
}
