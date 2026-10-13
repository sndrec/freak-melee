# Linked hitlag

`hitlag.active` observes the physical Fighter's native hitlag flag. The linked
operations expose the source-Fighter relation; they do not run hitlag callbacks
or decide which Fighters should be linked.

| Surface | Contract |
| --- | --- |
| `hitlag.active` | Read-only Bool: native `x2219_b5`, including a retained stock or independently authored Fighter receiver. |
| `hitlag.linked_id` | Read-only Int: the current peer creation ID, preserving its signed 32-bit bit pattern; zero if the peer is absent, stale or unavailable. |
| `hitlag.link(handle)` | Bool: assign the source's peer relation; zero clears it, a missing peer or self-link returns false without changing the relation. |
| `hitlag.unlink()` | Void: clear only the source's peer pointer and creation stamp. |
| `hitlag.request_release()` | Void: native release request; set `x221A_b1` only while `x2219_b7` is set. This does not directly clear `hitlag.active`. |

A link is one-sided. Authored code owns reciprocal release/unlink and its order.
Reading a stale linked ID does not clear the stored pointer. Foreign stock
Fighters are valid peers; an interpreted peer retains its own current program
and storage, without requiring the caller's bank.

An Article can read `attachment_owner.hitlag.active`. The receiver retains the
actual physical Fighter and, when interpreted, its own current program and
storage. This read does not require the caller's bank or source motion layout,
inspect the linked peer, or change native fields. A missing, retired or changed
receiver rejects without publishing a value. `linked_id`, `link`, `unlink` and
`request_release` require the current owned source Fighter; they are unavailable
through a physical receiver.

Handles must be integral native 32-bit values. Signed high-bit Int patterns are
preserved; wider integers outside `INT32_MIN..UINT32_MAX` are errors rather than
truncated IDs. Malformed declared Int/UInt/Bool values also reject. Link selection
uses the live Fighter registry and retains scene, pool and native creation
identity before writing. Failed lifetime/schema checks preserve result outputs.

Native peer relation fields belong to the Fighter's native snapshot. Restoring
only packed interpreted storage does not restore that native relation. These
calls use no native pointers from scripts and allocate no memory.
