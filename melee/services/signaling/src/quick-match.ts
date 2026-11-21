import { encodeMatchProposal } from "./match-proposal";
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
  const teams = new Map(allocation.players.map(player => [player.peerId, player.team]));
  const doubles = allocation.mode === 2 ? 1 : 0;
  const proposal = encodeMatchProposal(people, digest, STAGES[allocation.stage], {
    mode: 1, time: QUICK_RULES.minutes, stockTime: QUICK_RULES.minutes, stocks: QUICK_RULES.stocks,
    itemMask: 0n, itemFrequency: 255, handicap: 0, damage: 10, teams: doubles,
    friendlyFire: doubles, pause: 0, score: 0, sdPenalty: 0,
  }, teams);
  if (!proposal) throw new Error("Quick match is not prepared");
  return proposal;
}
