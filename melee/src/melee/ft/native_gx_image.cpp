#include <wincodec.h>
#include <wrl/client.h>
#include <malloc.h>
#include <cstdlib>

#include "native_gx_image.h"

// Decode at fighter activation, never during simulation or drawing. Aurora's
// GX texture upload expects GameCube-style RGBA8 tiles, not linear RGBA.
extern "C" unsigned char* nativeGxImage_Decode(
    const unsigned char* png, unsigned size, unsigned short* out_width,
    unsigned short* out_height)
{
    using Microsoft::WRL::ComPtr;
    if (!png || !size || !out_width || !out_height) return nullptr;
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
        SUCCEEDED(stream->InitializeFromMemory(const_cast<BYTE*>(png), size)) &&
        SUCCEEDED(factory->CreateDecoderFromStream(stream.Get(), nullptr,
                                                   WICDecodeMetadataCacheOnLoad,
                                                   &decoder)) &&
        SUCCEEDED(decoder->GetFrame(0, &frame)) &&
        SUCCEEDED(frame->GetSize(&width, &height)) &&
        width > 0 && width <= 1024 && height > 0 && height <= 1024 &&
        SUCCEEDED(factory->CreateFormatConverter(&converter)) &&
        SUCCEEDED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA,
                                        WICBitmapDitherTypeNone, nullptr, 0,
                                        WICBitmapPaletteTypeCustom));
    unsigned char* rgba = nullptr;
    unsigned char* tiled = nullptr;
    if (ready) {
        const unsigned padded_width = (width + 3) & ~3u;
        const unsigned padded_height = (height + 3) & ~3u;
        const size_t pixel_bytes = size_t(width) * height * 4;
        rgba = static_cast<unsigned char*>(malloc(pixel_bytes));
        tiled = static_cast<unsigned char*>(
            _aligned_malloc(size_t(padded_width) * padded_height * 4, 32));
        if (rgba && tiled &&
            SUCCEEDED(converter->CopyPixels(nullptr, width * 4,
                                            UINT(pixel_bytes), rgba))) {
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
            *out_width = static_cast<unsigned short>(width);
            *out_height = static_cast<unsigned short>(height);
        } else {
            _aligned_free(tiled);
            tiled = nullptr;
        }
    }
    free(rgba);
    return tiled;
}

extern "C" void nativeGxImage_Free(unsigned char* pixels)
{
    _aligned_free(pixels);
}
