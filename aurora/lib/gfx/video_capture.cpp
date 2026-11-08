#include "video_capture.hpp"
#include "../internal.hpp"
#include <cstdio>
#include <cstring>
#include <SDL3/SDL_timer.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

namespace aurora::gfx::video_capture {
namespace {
constexpr Module Log{"video-capture"};
Command pending;
// The render lane owns the stream and allocation. Readback completes on that
// same lane before advancing, so frames remain ordered with bounded memory.
struct Capture {
  FILE* stream = nullptr;
  wgpu::Buffer buffer;
  std::vector<unsigned char> rgb;
  uint32_t width = 0, height = 0, pitch = 0;
  uint64_t frames = 0;
  bool failed = false;
} capture;

void close() {
  if (!capture.stream) return;
  std::fclose(capture.stream); capture.stream = nullptr;
  Log.info("Stopped after {} frames; failed={}", capture.frames, capture.failed);
  capture.buffer = {}; capture.rgb.clear();
}

FILE* open(const char* path) {
#ifdef _WIN32
  if (!std::strncmp(path,"\\\\.\\pipe\\",9)) {
    HANDLE pipe = CreateNamedPipeA(path,PIPE_ACCESS_OUTBOUND,PIPE_TYPE_BYTE | PIPE_NOWAIT,
                                   1,1<<20,0,0,nullptr);
    if (pipe == INVALID_HANDLE_VALUE) return nullptr;
    Log.info("Listening {}",path);
    bool connected=false;
    for (unsigned wait=0;wait<2000;++wait) {
      if (ConnectNamedPipe(pipe,nullptr) || GetLastError()==ERROR_PIPE_CONNECTED) { connected=true; break; }
      if (GetLastError()!=ERROR_PIPE_LISTENING) break;
      SDL_Delay(5);
    }
    DWORD mode=PIPE_READMODE_BYTE | PIPE_WAIT;
    if (!connected || !SetNamedPipeHandleState(pipe,&mode,nullptr,nullptr)) { CloseHandle(pipe); return nullptr; }
    int descriptor = _open_osfhandle(reinterpret_cast<intptr_t>(pipe),_O_WRONLY | _O_BINARY);
    if (descriptor < 0) { CloseHandle(pipe); return nullptr; }
    FILE* file = _fdopen(descriptor,"wb");
    if (!file) _close(descriptor);
    return file;
  }
#endif
  return std::fopen(path,"wb");
}
}

bool request(const char* path) {
  if (pending.operation || (path && (!*path || std::strlen(path)>=pending.path.size()))) return false;
  pending.operation = path ? 1 : 2;
  if (path) std::strcpy(pending.path.data(),path);
  return true;
}

Command take_request() { Command result=pending; pending={}; return result; }

void encode(const wgpu::CommandEncoder& encoder, const Command& command,
            const wgpu::Texture& texture, wgpu::TextureFormat format,
            const wgpu::Extent3D& extent,
            std::vector<std::function<void()>>& afterSubmit) {
  if (!capture.stream && !command.operation) return;
  if (command.operation) {
    close();
    if (command.operation==2) return;
    capture.failed=false; capture.frames=0;
    capture.width=extent.width; capture.height=extent.height;
    capture.pitch=(extent.width*4+255)&~255u;
    capture.stream=open(command.path.data());
    if (!capture.stream) { Log.error("Could not open {}",command.path.data()); return; }
    capture.rgb.resize(size_t(extent.width)*extent.height*3);
    const wgpu::BufferDescriptor descriptor{
      .label="Video readback",.usage=wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead,
      .size=uint64_t(capture.pitch)*extent.height};
    capture.buffer=webgpu::g_device.CreateBuffer(&descriptor);
    Log.info("Started {}x{} -> {}",extent.width,extent.height,command.path.data());
  }
  if (!capture.stream) return;
  bool bgra=format==wgpu::TextureFormat::BGRA8Unorm || format==wgpu::TextureFormat::BGRA8UnormSrgb;
  bool rgba=format==wgpu::TextureFormat::RGBA8Unorm || format==wgpu::TextureFormat::RGBA8UnormSrgb;
  if (capture.failed || !texture || (!bgra && !rgba) || extent.width!=capture.width ||
      extent.height!=capture.height || !capture.width || !capture.height) {
    capture.failed=true; close(); return;
  }
  const wgpu::TexelCopyTextureInfo src{.texture=texture};
  const wgpu::TexelCopyBufferInfo dst{.layout={.bytesPerRow=capture.pitch,.rowsPerImage=extent.height},.buffer=capture.buffer};
  encoder.CopyTextureToBuffer(&src,&dst,&extent);
  afterSubmit.push_back([bgra] {
    const uint64_t size=uint64_t(capture.pitch)*capture.height;
    auto future=capture.buffer.MapAsync(wgpu::MapMode::Read,0,size,wgpu::CallbackMode::WaitAnyOnly,
      [bgra,size](wgpu::MapAsyncStatus status,wgpu::StringView) {
        bool ok=false;
        if (status==wgpu::MapAsyncStatus::Success) {
          auto* source=static_cast<const unsigned char*>(capture.buffer.GetConstMappedRange(0,size));
          if (source) {
            const unsigned red=bgra?2:0, blue=bgra?0:2;
            for (uint32_t y=0;y<capture.height;++y) {
              auto* row=source+size_t(y)*capture.pitch;
              auto* out=capture.rgb.data()+size_t(y)*capture.width*3;
              for (uint32_t x=0;x<capture.width;++x) {
                out[x*3]=row[x*4+red]; out[x*3+1]=row[x*4+1]; out[x*3+2]=row[x*4+blue];
              }
            }
            ok=std::fprintf(capture.stream,"P6\n%u %u\n255\n",capture.width,capture.height)>0 &&
              std::fwrite(capture.rgb.data(),1,capture.rgb.size(),capture.stream)==capture.rgb.size();
            if (ok) ++capture.frames;
          }
          capture.buffer.Unmap();
        }
        if (!ok) capture.failed=true;
      });
    // WaitAny dispatches the map callback here; sleeping on a completion flag
    // would prevent Dawn's event pump from making progress on this render lane.
    auto status=webgpu::g_instance.WaitAny(future,5000000000);
    if (status!=wgpu::WaitStatus::Success) Log.fatal("Video readback did not complete");
  });
}

void shutdown() { close(); }
}
