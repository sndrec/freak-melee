# Native disc assets

Files in `assets/disc` are mounted at the same paths as files on the game
disc. For example, `audio/first_startup.hps` is available as
`/audio/first_startup.hps`. The build copies the directory beside the executable
and includes it in the release file list. The original ISO stays unchanged.

`audio/first_startup.hps` was converted from the user's
`A:/randomfiles/mode_select.flac` for first startup. It is stereo GC-ADPCM at
44,100 Hz, with a full-track loop. The normal HPS streaming player handles it.

Conversion tools: FFmpeg and [VGAudioCli 2.2.1](https://github.com/Thealexbarney/VGAudio/releases/tag/v2.2.1).
They are authoring tools; the client runs without them.

```powershell
ffmpeg -i mode_select.flac -acodec pcm_s16le mode_select.wav
VGAudioCli mode_select.wav first_startup.hps -l 0-4512671
```
