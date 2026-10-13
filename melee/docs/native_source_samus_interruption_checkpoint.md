# Samus shared interruption checkpoint

The 2026-10-10 batch supplies generic
[`damage.interrupt_cleanup()`](scripting/fighter_observations.md#damageinterrupt_cleanup)
for an owned Fighter. It directly executes the shared engine interruption
routine: stop the two voice channels, invoke damage cleanup, then invoke
primary-death cleanup. It does not delegate to a stock special handler or
merely clear callback registrations. Physical generation, selected program,
storage and scene are checked before execution and again before publication.
Faulting or detaching the source does not publish a successful result.

Samus's bomb-jump helper uses this operation and now prepares. Its launch
velocity also follows `ftSamus_80128B1C_inner`: speed times cosine/sine,
without an additional model-scale multiplier. The clamp remains authored
separately as before. No original gameplay routine was changed.

Evidence is stored at
`A:/FreakMelee-test-archives/samus-interruption-checkpoint-oct10-a/receipt.json`.
The connected observation cohort runs the production binding helper and
selected Combat dispatch prefix, actual compiler/interpreter/actor storage,
and unchanged native cleanup/voice-stop bodies. It covers 64 callback/slot
combinations, native-pointer override precedence, exact stop command arguments,
source detachment, interpreted callback faults and invalid invocation contexts.
Native fallback callbacks and audio are controlled recording leaves; unrelated
Combat branches remain unavailable. This is not real-audio or bomb-jump game
acceptance. Seven native targets build; both focused scripting CTests pass.

The complete 172-body authoring audit reports two failing bodies: missile
target search and bomb-owner overlap. Their subsequent operations may expose
additional unsupported calls, including recipient helpers with arguments.
Samus remains rejected and has no accepted independent package. Grapple,
remaining lifecycle ownership and real match comparisons remain open. No
user collection change or visible/audible game launch occurred.
