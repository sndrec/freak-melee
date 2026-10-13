# Fixed shape interpolation scratch

The runner freezes actual current and prior `pobj.c` interpolation bodies and
their unchanged shape decoding helpers. Both actual display-list interpreters
emit through controlled GX leaves, alongside copying consumed prefixes.
Prior `HSD_MemAlloc` calls use counted malloc; the fixed path
must allocate nothing and reproduce every captured float bit after poisoning
all its scratch before every draw.

Coverage: average/additive weights, clamped average selection, negative additive
weights, 8/16-bit index tables, big/native-endian float arrays, normal/NBT data,
and vertex-only draws alternating with normal draws. Original capacity failures
remain failures, including negative counts. Position, normal and NBT display
indices outside the regenerated prefix refuse in the fixed path. No GPU/pixel, complete
animation, allocator topology or whole-match timing claim.

Pass a frozen pre-change `pobj.c` with `--baseline`, LLVM clang with `--compiler`,
and a fresh A: archive directory with `--output`. The archive stores commands,
source pins, copied headers, generated bodies, executable and receipt. No game
or window is launched.
