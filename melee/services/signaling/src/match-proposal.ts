import { lockedChoices } from "./lobby-selection";
import type { LobbyRules } from "./lobby-rules";
import type { Participant } from "./protocol";

type MatchRules = Omit<LobbyRules, "stagePolicy" | "stageMask">;
export const MATCH_PEER_COUNT_OFFSET = 60;

// FMLM uses the same port order as nativeLobbyTopology_AssignMatchPeers.
// An empty port belongs to peer zero for simulation input.
export function encodeMatchProposal(people: Participant[], digest: Uint8Array,
  stage: number, rules: MatchRules, assignedTeams?: ReadonlyMap<string, number>): Uint8Array | null {
  const fighters = people.filter(person => person.activeMask).sort((a, b) => a.joinOrder - b.joinOrder);
  if (digest.length !== 32 || !digest.some(byte => byte) || !fighters.length || fighters.length > 4)
    return null;
  const bytes = new Uint8Array(768);
  const view = new DataView(bytes.buffer);
  let offset = 0;
  const u8 = (value: number) => { bytes[offset++] = value; };
  const u64 = (value: bigint) => { view.setBigUint64(offset, value); offset += 8; };
  const raw = (value: Uint8Array) => { bytes.set(value, offset); offset += value.length; };
  const encoder = new TextEncoder();
  raw(encoder.encode("FMLM"));
  const session = new BigUint64Array(1);
  do { crypto.getRandomValues(session); } while (!session[0]);
  u64(session[0]); raw(digest);
  u64(0x464d535400000000n | BigInt(stage + 1));
  u64(rules.itemFrequency === 255 ? 0n : rules.itemMask);
  u8(fighters.length); u8(rules.mode); u8(rules.time); u8(rules.stockTime);
  u8(rules.stocks); u8(rules.itemFrequency); u8(rules.handicap); u8(rules.damage);
  u8(rules.teams | rules.friendlyFire << 1 | rules.pause << 2 | rules.score << 3 | rules.sdPenalty << 4);
  const peers: { id: Uint8Array; mask: number }[] = [];
  let port = 0;
  for (const person of fighters) {
    const choices = lockedChoices(person.selection, person.localPlayers, person.activeMask);
    const id = encoder.encode(person.peerId);
    const team = assignedTeams?.get(person.peerId);
    if (!choices || !id.length || id.length > 64 || (assignedTeams && team === undefined)) return null;
    let mask = 0;
    for (let seat = 0; seat < choices.length; ++seat) {
      if (!(person.activeMask & (1 << seat))) continue;
      if (port === 4) return null;
      const choice = choices[seat];
      mask |= 1 << port++;
      u8(1); u8(seat); u8(0); u8(team ?? choice.team); u8(9);
      u64(choice.character); u64(choice.costume);
      raw(choice.fighterPackage); raw(choice.costumePackage);
    }
    peers.push({ id, mask });
  }
  if (port < 2) return null;
  peers[0].mask |= 15 & ~((1 << port) - 1);
  offset += (4 - port) * 85;
  for (const [index, peer] of peers.entries()) {
    u8(index); u8(peer.mask); u8(peer.id.length); raw(peer.id);
  }
  return bytes.slice(0, offset);
}
