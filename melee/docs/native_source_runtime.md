# Source package registration

Ordinary custom fighter registration consumes the packet's owned asset record,
fighter/animation/costume DATs, FMS source, immutable parameter text and CPU
profile. The packet member key identifies the authoring member; it never selects
a donor implementation or a DLL. `nativeSourcePackage_Prepare` builds the actual
typed program and imports the assets at their final stable heap address.

Single and AlternateForm source families can activate. Each alternate member
must provide its own `form_activated` event. SimultaneousCompanion families are
still rejected before publication pending their member and CPU-history
contracts. Workshop items require pinned named resources and their own
DAT/model blueprints. There is no missing-resource or missing-event stock
fallback.

`nativeFighterRuntime_StageCached` prepares all requested candidates before
replacing previous staging. Failed reads, imports, FMS compilation or CPU parsing
leave accepted registrations and previous staging intact. Candidate source
packages never move; the plain registration descriptors can move between the
two banks at the transition. Preparation may allocate; execution does not.

`nativeFighterRuntime_ActivateStaged` validates all candidates, real common data,
publication eligibility and absence of retained rollback history before taking
the runtime leases. After all leases succeed, it installs the actual imported
character, costume, rig, numeric motions, demo names, animation counts, sound
facts and CPU tables, then publishes the prepared code-ID programs. Native
authored callback tables are cleared. The program registry copies and rebinds
the prepared immutable program; its bank and identity identify the candidate.

The selected source is available through `nativeFighterRuntime_SourcePackage`.
`nativeFighterRuntime_Definition` is null for ordinary source registration.
Rig mapping, surface-friction policy, sound-ID/KO policy, channel muting, pose
graphs and animation-family policy consume selected owned facts directly.
Pose validation checks the current imported data, animation arena and selected
costume. Common results clips use the owned demo bank; a source package without
a native custom demo selector rejects custom demo variants before construction.
Source trails have no native callback fallback.

Generic state and motion transitions validate the actual selected source actor,
not a callback-bearing Definition. The binding validator requires the selected
package's imported data/motion table, matching published bank and layout, and a
live actor storage view with the current generation. Numeric targets must lie
inside that layout's common or owned interval. Wait/fall/landing operations use
the same ownership boundary; source actors cannot enter a donor policy branch.
Collision/control and animation continuation barriers also stop when this view
retires or faults. Common-only diagnostic programs without a prepared package
cannot perform these owned state transitions.

`nativeFighterRuntime_CanClear` requires an idle VM, no executing/deferred GObj
work, no current source actors or suspended actor frames, exactly the runtime's
own lease, and the exact selected program bank. It also rejects while
`nativeRollbackPages_IsInitialized` reports the actual tracker allocations:
completed frames retain package addresses even when no frame is being written.
Shutting down the tracker, rather than merely ending its active frame, releases
this history constraint.

`nativeFighterRuntime_Clear` performs that complete preflight before mutating
anything. A rejected retirement preserves the package, program and all asset
tables. A permitted retirement clears the selected program and tables, releases
the runtime lease and only then frees the package. Staged next-scene candidates
remain separate. External leases and exact actor-bank references cannot be
silently invalidated by scene initialization.

The actual runtime transaction fixture is
`tests/native/source_package_runtime/test.c`. It links the real runtime and
uses a newly generated real packet/cache, checking publication, failed candidate
isolation, external leases and a completed Windows page-tracker frame. It does
not create actors or establish gameplay fidelity; direct match acceptance is a
separate engine test.
