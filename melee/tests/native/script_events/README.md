# Actual script event contracts

```powershell
python tests/native/script_events/run.py --output A:/FreakMelee-test-archives/script-events-oct07
```

The fixed native catalogue in `native_script_events.c/h` resolves events using
actual source parents and ownership. Fighter motion phases return Void;
article state animation/collision return Bool and physics returns Void. Named
fighter policies have audited Bool/Int/Float/Void signatures. Workshop item
hit/ground-collision predicates and effect updates have distinct contexts.
The catalogue retains native slot/signature/source evidence and typed input
payload properties. Incoming object pointers become retained identity/generation
references; mutable native payloads are not accepted as guessed script values.

Source evidence includes the actual native fighter definition, ItemStateTable,
ItemLogicTable and callback typedefs, checked against the reference emitters.
The source checker verifies all 188 named slots/function-argument hooks exist
in their actual headers. Stage hooks are actual collision-operation callback
arguments, not invented Item fields.

The semantic event identity is separate from an authored fighter motion number
or article signal name. Article state header syntax/ranges are validated for
the signature's state identity. Fighter motion IDs still need validation against
the loaded package's motion table. All other asset/metadata validation and
callback scheduling remain separate acceptance gates. Unsupported declarative
headers (`input ...`, `setup ...`, extra signal arguments), incorrect source
placement and unknown events fail explicitly. No such input/setup event
contract was found in the current authoring/compiler sources.

Callbacks with source-audited fallthrough results emit explicit bytecode:
false, zero, -1 or the specified incoming parameter. Held motion, air-resume,
victim motion, action metadata and aerial-attack motion preserve their audited
input. Event parameters have anonymous local slots; descriptor properties map
to read-only parameter semantics. An authored shadowing local cannot replace
the incoming payload. Typed helpers retain their strict return requirement.

Bare `return` in typed callbacks remains an error: the reference fighter parser
permits it only outside predicate callbacks, while article typed/predicate
routes reject it. Void event early returns remain supported. Article held-status
has an audited read-only body policy: mutation, loops, authored helpers and
unmarked native calls are rejected by the generic statement policy. Pure native
call metadata is required for later allowed query/math families.

Audited exact Int/Bool return-expression requirements use a generic StrictReturn
policy; ordinary numeric Bool truth uses TruthReturn. These are distinct from
the function's native result type. CPU prepare/stick/mode/output/drop, held and
air-resume mapping, and article held-status/request retain their exact checks.

The focused fixture indexes 28 callbacks, compiles their actual payload types
and defaults through ProgramIndex/Statement/Bytecode/VM, and checks actual
outputs including high-bit history identity and explicit returns that override
normal fallthrough. It distinguishes ownership using same-spelled state phases.
Three ordinary Bool helpers also execute numeric returns 1, 0.5 and 0 using the
documented nonzero truth rule. This changes return conversion only; Bool
property assignment, parameters and declaration defaults retain their current
separate validation contracts.
The broader census finds 1,861 resolved callback signatures across the 16
selected sources and two explicit unsupported trail callbacks in Link/Young
Link. `NativeFighterTrail*` has mutable output semantics requiring an accepted
script/output contract; it is not converted into an Object placeholder.

The archive contains `manifest.json`, `receipt.json` and compiler/test logs.
This establishes signatures and focused interpreted behavior, not whole-template
typed acceptance, engine/scheduler integration or gameplay parity. All subprocesses
run with CREATE_NO_WINDOW on Windows; no game is launched.
