# Grapple descriptor native-layout regression

Run `run.py --compiler <clang.exe> --output <fresh directory>` from the repository.
This is a console-only fixture; it opens no game window and produces no audio.

The runner extracts the current grapple initializer and the original initializer
at `1ba857e70`, the unchanged stock descriptor, and the actual common group and
victim-history routines. A separate reference decodes explicitly packed GameCube
command words. The stock case and 8,192 varied descriptors across eight initial
capsule-state/group combinations compare all four capsule byte images, the
cleanup marker, and damage/angle/bone-conversion boundary arguments and order.
Both ordinary and common bone IDs, signed offsets/shield damage, descriptor
flags, retained history, group reset and copied history are exercised.

The old initializer reads host byte offset 2 as if it were a GameCube command
halfword. The stock descriptor therefore supplies 17 damage on Windows instead
of its declared 0. The corrected initializer uses named fields. Grounded/aerial
flags and the exact `0.003906f` coefficient remain unchanged. Three real creation
sites now initialize the trailing flags to zero; retail's uninitialized
victim-only bit is intentionally made deterministic and false.

Damage/angle helpers and common-bone conversion are controlled leaves. This
proves descriptor decoding and initialization, not damage scaling/staleness,
live resource lifetime, script creation or complete Samus gameplay. Call-site
initialization is checked statically. No independent-Samus registration is made.
