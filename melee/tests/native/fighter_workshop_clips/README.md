# Workshop Action/Demo clip selection

```powershell
python tests/native/fighter_workshop_clips/run.py --output A:/FreakMelee-test-archives/workshop-clips-review
```

The headless fixture links actual editor_clips.cpp, editor_session.cpp, labels.cpp and ScriptFiles.cpp plus exact extracted select_mode, select_action and apply_history bodies from the current checkout. It uses a real RmlUi document/context, controls, generated rows and selected classes. Synthetic decoded clip metadata and graph/model/preview presentation are controlled boundaries; gameplay decode calls are counted. No game, import, save, build, installation or runtime demo rendering occurs.

Checks distinguish Action 0 from Demo 0, exclude undecoded demos, match mixed-case filters, verify highlights, and prevent direct demo selection from decoding gameplay commands/events or changing selected_action. Action/Animation mode changes preserve an independent demo/frame; crossing bank with the same numeric index resets the new clip frame. Actual two-member session selection retains separate clip/frame/filter state. Actual undo/redo restores both DAT buffers while retaining the selected demo, frame and gameplay action, and leaves the other member's history untouched.

The runner captures source hashes before compilation and rejects concurrent changes. It extracts function bodies as complete original text, writes all outputs to a fresh A: directory, uses vcvars 14.44 and hidden subprocesses. RmlUi/freetype/zlib libraries come from the existing native dependency build. The minimal RML contains the IDs used by those actual functions; it does not claim the entire shipped Workshop layout was rendered.

Three exact extracted click branches additionally cover graph-key, dope-key and Select visible. Controlled Action 0 keys have values 1/2 at frames 5/8; Demo 0 has 101/102/103 at frames 3/17/41. The fixture verifies selected Demo key data, shift selection, visibility/frame filtering and the distinct Action results through the same branches. Branch text is unchanged; only its test function enclosure supplies id/shift arguments.
