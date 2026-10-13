# Fighter movement scheduler fixture

The real source frontend, hook contract table, preparation, Fighter actor, VM/storage and registered fixed GObj pool execute eight callbacks: held_motion, air_resume_motion, neutral_enter, wait_enter, grounded_motion_setup, held_timer_expired, aerial_action_manual and aerial_action_automatic.

The extractor preserves complete held-motion, air-resume, neutral/wait and aerial arbitration functions, both Parasol entry functions and the native Parasol timer helper. Grounded motion setup and held-timer expiry are exact blocks from fighter.c; surrounding unrelated motion/tick stages are omitted. This fixture does not replace nativeScriptFighter_Run with a successful stub.

The matrix checks exact signed motion/previous-motion arguments, owned-layout acceptance and invalid-map fallback, F32 blend, notification/common-tail ordering, handled/missing decisions, timer flag order, native resume-function selection, and source Peach bypassing stock Peach policy. Unchanged HEAD held/air-resume/neutral/aerial bodies on an unowned Mario GObj independently sample the preserved common fallback. The archive retains the original HEAD hashes and extracted source bodies.

There are 54 scheduler lifetime cases plus ten state intrinsic cases: physical deletion, same-slot reuse, userdata replacement, native generation change, and actual VM write failure. Wait cleanup and ParasolOpen's continuation after grounded setup/animation are included. Native animation leaves exercise four physical mutations; logical faults are proved through authored grounded setup, rather than pretending a void animation leaf returned a VM error. Null/unregistered held-selector entry is rejected before userdata access.

Selected source package/table publication and common motion, item, animation, player and geometry leaves are controlled recording boundaries. The fixture's synthetic immutable facts are not real DAT/rig acceptance. Position writes deliberately inject mutation at an observation point; this is not a claim that native field reads naturally invoke callbacks. No game, rendering or whole Fighter_ChangeMotionState/tick acceptance is claimed. Unrelated binding families explicitly reject.

Run `python run.py --compiler <clang.exe> --output A:/FreakMelee-test-archives/<fresh-name>`. All subprocesses remain hidden. Receipt fingerprints native bodies, shared compiler/core units and headers. Production focused syntax and the fixture translation units use strict warnings with only the known Dolphin array-parameter declaration exception.

The intrinsic cases execute authored `state.wait()` and `state.wait_blend()`
helpers through the real descriptor, VM/native marshalling and production
nativeScriptBindingState_Invoke. A separately prepared CustomFirst source
program satisfies the actual imported-layout guard; the source Peach actor
continues to prove stock-policy bypass only at the native scheduler entries.
Unrelated state primitives are explicitly unavailable in this fixture. Both
positive intrinsic calls must finish their authored tail, while deletion/reuse,
userdata/generation change and actual VM failure during WaitEnter suppress it.
