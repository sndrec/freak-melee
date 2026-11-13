# Native screenshots

F12, the original Y + D-pad Up debug request, and the native control channel's
`screenshot` command save the last completed presented frame. The file is written to
`screenshots/` under the working directory as
`melee-<time>-<process>-<sequence>.png`. The capture includes the scaled Melee
image and both native UI sidebars, but excludes window chrome and the desktop.

`aurora_capture_presented` reserves the most recently presented image on the
game thread. A fixed set of three GPU textures holds the displayed image, a
pending presentation, and an optional reserved capture. A reserved texture
stays intact while later frames render, including across window resizing.
Textures are allocated at startup, resize, or first use of the third slot.

The existing render worker reads that reserved image and encodes an RGB PNG
with libpng. No additional worker thread is created. Readback storage is
allocated only on request. Null, empty, non-PNG and overlapping requests are
rejected. Acceptance reserves the image; `aurora_presented_capture_status`
reports write completion or failure, and errors also reach the log. BGRA8 and
RGBA8 surfaces, including their sRGB variants, are supported.

The retained-image form supports F2 feedback. It returns the displayed frame
ID, presentation timestamp and pixel dimensions at activation. The retained
image can also supply a frozen full-window backdrop for feedback annotation.
The image is released explicitly when annotation ends. F12 releases its image
automatically after readback and writing.

The render-worker lifetime and submission order follow the project's
Multi-Core By Default guideline:
https://www.dgtlgrove.com/p/multi-core-by-default

## Evidence

The 2026-11-13 D3D12 capture test reserved a red 64x32 image, queued eight newer
green 128x64 images before readback, and verified that the PNG retained the red
pixels and original dimensions/frame ID. It also checked export of the same
reserved image, release, cancellation, unsupported paths and PNG-write failure.
A hidden native client captured its complete 1424x861 interface through F12's
control-channel path after the renderer change.

A live D3D12 client captured
`screenshots/melee-1790211932-36280-0000.png` through the control channel.
The 1440x810, 496,533-byte PNG was opened and inspected: the character-select
screen, left navigation and controller list, and right room/chat sidebar are
all present. The log reports a successful save. This verifies the full client
capture on this machine; other graphics backends and readback failure paths
remain untested. Existing BMP screenshots were left untouched.
