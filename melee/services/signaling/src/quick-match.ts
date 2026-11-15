import { lockedChoices } from "./lobby-selection";
import type { Participant } from "./protocol";
import { QUICK_RULES, type QuickAllocation } from "./quick-play";

export const QUICK_COUNTDOWN_MS = 3000;
// Native St_Kind values, in QUICK_STAGES order.
const STAGES = [0x1f, 0x20, 0x02, 0x1c, 0x03, 0x08];
export function quickRosterComplete(allocation: QuickAllocation, people: Participant[]): boolean {
  return people.length === allocation.players.length && people.every((person) => {
    const assigned = allocation.players.find((player) => player.peerId === person.peerId);
    return assigned && person.build === allocation.build && person.localPlayers === assigned.localPlayers &&
      person.activeMask === (1 << assigned.localPlayers) - 1 && person.waitingMask === 0 &&
      lockedChoices(person.selection, person.localPlayers) !== null;
  });
}

// Construct the same FMLM proposal consumed by the native lobby handshake.
// Canonical ports follow presence join order; a shared PC owns consecutive seats.
export function buildQuickMatch(allocation: QuickAllocation, people: Participant[], digest: Uint8Array): Uint8Array {
  if (!quickRosterComplete(allocation, people) || digest.length !== 32 || !digest.some((b) => b !== 0) ||
      allocation.stage < 0 || allocation.stage >= STAGES.length) throw new Error("Quick match is not prepared");
  const bytes = new Uint8Array(768);
  const view = new DataView(bytes.buffer);
  let offset = 0;
  const u8 = (value: number) => { bytes[offset++] = value; };
  const u64 = (value: bigint) => { view.setBigUint64(offset, value); offset += 8; };
  const raw = (value: Uint8Array) => { bytes.set(value, offset); offset += value.length; };
  raw(new TextEncoder().encode("FMLM"));
  const random = crypto.getRandomValues(new Uint32Array(2));
  u64((BigInt(random[0]) << 32n | BigInt(random[1])) || 1n);
  raw(digest);
  u64(0x464d535400000000n | BigInt(STAGES[allocation.stage] + 1));
  u64(0n); // Items off.
  u8(people.length); u8(1); u8(QUICK_RULES.minutes); u8(QUICK_RULES.minutes);
  u8(QUICK_RULES.stocks); u8(255); u8(0); u8(10);
  u8(allocation.mode === 2 ? 3 : 0); // Teams and friendly fire in doubles.
  let port = 0;
  const peers: { id: string; mask: number }[] = [];
  for (const person of people) {
    const assigned = allocation.players.find((player) => player.peerId === person.peerId)!;
    const choices = lockedChoices(person.selection, person.localPlayers)!;
    let mask = 0;
    for (let seat = 0; seat < choices.length; ++seat) {
      const choice = choices[seat];
      mask |= 1 << port++;
      u8(1); u8(seat); u8(0); u8(assigned.team); u8(9);
      u64(choice.character); u64(choice.costume);
      raw(choice.fighterPackage); raw(choice.costumePackage);
    }
    peers.push({ id: person.peerId, mask });
  }
  while (port < 4) { raw(new Uint8Array(85)); ++port; }
  peers[0].mask |= 15 & ~((1 << (allocation.mode * 2)) - 1);
  for (let i = 0; i < peers.length; ++i) {
    const id = new TextEncoder().encode(peers[i].id);
    u8(i); u8(peers[i].mask); u8(id.length); raw(id);
  }
  return bytes.slice(0, offset);
}
