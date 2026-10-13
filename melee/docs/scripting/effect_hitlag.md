# Effect hitlag controls

`effects.pause()` and `effects.resume()` use the common native effect loops on
both effect lists. Matching `parent_gobj` records enter pause or resume while
retaining their async bit. `effects.destroy_all()` destroys matching effects;
`effects.clear_all()` also clears the Fighter's active-effect flag first.

`effects.pause_during_hitlag()` requires an attached source Fighter. It clears
the saved authored pre/post hitlag callback IDs, then installs the shared native
pause/resume callbacks. Authored `callbacks.pre_hitlag(Helper)` and
`callbacks.post_hitlag(Helper)` can replace them with owned zero-argument Void
helpers. Explicit clearing uses `callbacks.pre_hitlag(None)` and
`callbacks.post_hitlag(None)`; there is no clear_hitlag alias.

The effect controls retain physical Fighter identity and, for attached actors,
their exact selected program/storage/schema. Native destruction may have completed
effects before the caller faults or retires; output is withheld and authored VM
continuation stops. Ordinary physical Fighter recipients can control their own
effects without sharing the caller's program. Callback installation requires the
recipient's own live interpreted actor.

The connected console cohort covers native loops and authored callback ownership.
Graphics scheduling, effect constructors and model-effect cancellation are
controlled leaves; this is not renderer or complete fighter acceptance.
