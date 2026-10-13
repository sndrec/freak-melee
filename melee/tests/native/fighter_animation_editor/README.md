# Explicit action/demo animation editing

Run from the checkout with a fresh A: output:

```powershell
python tests/native/fighter_animation_editor/run.py --output A:/FreakMelee-test-archives/animation-editor-demo-review
```

The fixture links the actual native animation editor, DAT reader, figatree decoder, FObj interpreter and spline implementation. Its synthetic DAT has independent action/demo descriptor tables, shared action/demo references, an empty demo, and three physical clip blobs. Stack-FObj sampling never allocates; allocator stubs abort if reached.

Coverage includes demo metadata/tracks/keys/sampling, values, spline tangents, frames, moving/inserting/deleting keys, and creating a new channel. Growth through either bank updates offset/size words in both descriptor tables, including aliases. Other clip bytes and descriptor command/flag/blend words remain unchanged. Invalid bank/index/empty demo rejects; a malformed secondary table makes a structural edit fail without changing either DAT buffer. The public clip reference is explicit; existing Workshop/pose/CLI callers select the Action bank. A demo UI selector is outside this backend gate.

In-memory structural history now captures both animation and fighter DAT data; pose paste works on private copies of both buffers before committing. Disk transaction/undo is owned by the project layer and tested separately. This fixture proves backend data editing, not runtime demo selection or rendering.

CMake recipe: executable with test.c plus src/melee/ft/native_fighter_animation_editor.c, src/melee/lb/lbanim_dat.c, src/sysdolphin/baselib/dat.c, fobj.c and spline.c. Include src and extern/dolphin/include; use ordinary MELEE_NATIVE console definitions. No imported ISO or ignored output dependency is required.
