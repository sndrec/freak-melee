# Typed recipient-helper argument checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Recipient calls now pass up to 30 Bool, Int, UInt or Float arguments to an
exported root helper. The prepared instruction captures the complete call
arity. The recipient validates its own declaration and applies ordinary
language conversions before executing its VM. Missing recipients or exports
retain the documented neutral result; a matching export with incompatible
arguments fails before executing the helper. Parameters use fixed stack
storage and preserve the existing caller/recipient generation, scene, program,
state-layout and retirement checks.

See [exported helpers](scripting/functions.md) and
[recipient operations](scripting/targets.md) for authoring details. There are
no fighter-specific arguments, mailbox variables or stock special-handler
calls. Samus now exports `bomb_jump(float center, float radius)`; connecting
bomb overlap and its retained control owner remains separate work.

Evidence: `A:/FreakMelee-test-archives/recipient-arguments-checkpoint-oct10-a/receipt.json`.
The connected recipient fixture records 2,995 checks across 374 cases, 57
top-level VM invocations, three independent banks and 18 lifetime mutations.
It exercises all four dispatch forms, Fighter and Article callers, maximum
arity, conversion matrices and rejected malformed calls. Controlled package
assets, runtime package lookup and position-write leaves delimit this host
fixture; it is not a real-game or whole-match acceptance test.

The broader compiler fixture records 133 checks and 16 ticks with actual
assembly, owned state and transactional publication. Seven native targets
build and both focused scripting CTests pass. The complete Samus audit checks
172 bodies and still rejects the bomb-overlap call. Grapple, lifecycle/reset
ownership and real-game comparisons remain open. No game launch, user
collection modification or independent Samus acceptance occurred.
