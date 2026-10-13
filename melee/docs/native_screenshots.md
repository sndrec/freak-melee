# Native screenshots

F12, the original Y + D-pad Up debug request, and the native control channel's
`screenshot` command request the next presented frame. The file is written to
`screenshots/` under the working directory as
`melee-<time>-<process>-<sequence>.png`. The capture includes the scaled Melee
image and both native UI sidebars, but excludes window chrome and the desktop.

`aurora_request_screenshot` copies the requested path on the game thread. The
existing render worker copies the final window surface after RmlUi and ImGui
have drawn, submits the copy with the frame, then maps the readback and encodes
an RGB PNG with libpng. The surface allows GPU copy-source access. No additional
worker thread is created, and readback storage is allocated only on request.
Null, empty, non-PNG and overlapping requests are rejected. Acceptance means a
request was queued, not that a file was saved; GPU and file errors are logged.
A submitted frame is needed to complete the request. BGRA8 and RGBA8 surfaces,
including their sRGB variants, are supported.

The render-worker lifetime and submission order follow the project's
Multi-Core By Default guideline:
https://www.dgtlgrove.com/p/multi-core-by-default

## Evidence

A live D3D12 client captured
`screenshots/melee-1790211932-36280-0000.png` through the control channel.
The 1440x810, 496,533-byte PNG was opened and inspected: the character-select
screen, left navigation and controller list, and right room/chat sidebar are
all present. The log reports a successful save. This verifies the full client
capture on this machine; other graphics backends and readback failure paths
remain untested. Existing BMP screenshots were left untouched.
