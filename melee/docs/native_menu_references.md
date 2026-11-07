# Original menu scenery

Updated: 2026-11-07. The native client can display the original ISO's menu
models, cameras, lights, animations and gallery background images for design
reference. This inspection mode draws the scenery without the hub controls.
It leaves the artwork unchanged.

With a native client running, send `ui-inspect menu-reference:<name>` through
its control channel. Use `menu-reference:off` to return to the hub.

| Name | Original artwork |
| --- | --- |
| `gallery-image-0` | Gallery's brown background, `ToyFigureBg3_sobjdesc` |
| `gallery-image-1` | Gallery's blue/green background, `ToyFigureBg5_sobjdesc` |
| `gallery-image-2` | Gallery's purple background, `ToyFigureBg6_sobjdesc` |
| `gallery` | Animated `ToyFigureBack_Top` model in `TyMnView.usd` |
| `collection` | Trophy room, `ToyDspBg_Top` in `TyMnDisp.usd` |
| `lottery` | Lottery backdrop, `ToyFigurePonBg_Top` in `TyMnFigp.usd` |
| `tournament-background` | Tournament's animated backdrop, model 5 in `GmTou1p.usd` |
| `tournament-1` | First tournament scene's authored objects |
| `tournament-2` | Second tournament scene's authored objects |

The tournament scene previews show their authored objects together. The real
menu callbacks control which panels appear, repeat rows and populate player
information; the previews do not execute that flow. Use the background preset
to inspect the backdrop on its own.

The ordinary `--retail-menu` launch path also opens the original Options,
Rumble and Sound menus. Rumble's native conversion now references its strings,
animation settings and model array directly. Its previous casts assumed a
particular linker layout for separate globals and failed on the PC build.

## Captures and remaining work

The hidden, muted native captures in `.local/native-background-reference-2`
cover all three gallery images, the animated gallery model, trophy room,
lottery, the isolated tournament background and both tournament scene previews.
The build passed in `.local/menu-reference-build-2.log`. The actual Options, Rumble and
Sound menus were captured in `.local/retail-options-reference-1`.

These captures establish which original artwork is available for the startup
design. They do not establish that the full trophy or tournament modes are
ported: those flows still use unconverted archive loaders. The final startup
background and simplified layout await the user's choice.
