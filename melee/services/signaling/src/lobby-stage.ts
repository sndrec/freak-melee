import type { Participant } from "./protocol";
import type { LobbyRules } from "./lobby-rules";
import { lockedChoices } from "./lobby-selection";

export interface LobbyStagePlayer { peerId: string; seat: number }

// Match ports use join order, then the local seat number, even on shared PCs.
export function lobbyStagePlayers(people: Participant[]): LobbyStagePlayer[] {
  return [...people].sort((a, b) => a.joinOrder - b.joinOrder).flatMap((person) =>
    Array.from({ length: person.localPlayers }, (_, seat) => ({ peerId: person.peerId, seat }))
      .filter((player) => (person.activeMask & (1 << player.seat)) !== 0));
}

export function lobbyStageChooser(people: Participant[], loser: LobbyStagePlayer,
  policy = 4, previousTurn: LobbyStagePlayer = { peerId: "", seat: 255 }): LobbyStagePlayer | null {
  const players = lobbyStagePlayers(people);
  if (policy === 0 || policy === 1 || policy === 2) return null;
  if (policy === 3) {
    const last = players.findIndex((player) => player.peerId === previousTurn.peerId && player.seat === previousTurn.seat);
    return players[(last + 1) % players.length] ?? null;
  }
  return players.find((player) => player.peerId === loser.peerId && player.seat === loser.seat)
    ?? players[0] ?? null;
}

// Stock SSS order, followed by each entry's Random Stage Switch bit (xA).
// The last stock table entry is the Random button, not a stage.
export const LOBBY_STAGES = [4,11,5,12,13,14,8,16,2,17,7,22,6,15,9,18,10,24,3,23,19,20,25,27,31,32,28,29,30];
const RANDOM_BITS = [0,12,1,13,2,14,4,16,5,17,6,18,3,15,9,21,8,20,7,19,11,22,10,23,24,25,26,27,28];
export interface LobbyStageHistory {
  orderedIndex: number; turn: LobbyStagePlayer; randomWait: number[];
}
export function emptyLobbyStageHistory(): LobbyStageHistory {
  return { orderedIndex: -1, turn: { peerId: "", seat: 255 }, randomWait: Array(LOBBY_STAGES.length).fill(0) };
}

export function automaticLobbyStage(rules: LobbyRules, history: LobbyStageHistory, random: number): number {
  if (rules.stagePolicy === 2) {
    history.orderedIndex = (history.orderedIndex + 1) % LOBBY_STAGES.length;
    return LOBBY_STAGES[history.orderedIndex];
  }
  if (rules.stagePolicy !== 1) return 255;
  const mask = rules.stageMask & 0x1fffffff;
  const enabled = (i: number) => !mask || !!(mask & (1 << RANDOM_BITS[i]));
  if (!history.randomWait.some((wait, i) => wait >= 0 && enabled(i))) history.randomWait.fill(0);
  while (!history.randomWait.some((wait, i) => wait === 0 && enabled(i)))
    history.randomWait = history.randomWait.map((wait) => wait > 0 ? wait - 1 : wait);
  const candidates = history.randomWait.flatMap((wait, i) => wait === 0 && enabled(i) ? [i] : []);
  const index = candidates[random % candidates.length];
  history.randomWait[index] = -1;
  if (index < 22 && history.randomWait[index ^ 1] >= 0) history.randomWait[index ^ 1] = 3;
  return LOBBY_STAGES[index];
}

// Only fighting seats affect readiness. Waiting/spectating PCs can edit their
// own selections without restarting an unrelated match countdown.
export function lobbyReadySignature(people: Participant[], rules: LobbyRules): string | null {
  const roster: string[] = []; let build = "", teams = 0;
  for (const person of [...people].sort((a,b) => a.joinOrder - b.joinOrder)) {
    if (!person.activeMask) continue;
    const choices = lockedChoices(person.selection, person.localPlayers, person.activeMask);
    if (!choices || build && person.build !== build) return null;
    build = person.build;
    for (let seat = 0; seat < person.localPlayers; ++seat) if (person.activeMask & (1 << seat)) {
      const choice = choices[seat]; teams |= 1 << choice.team;
      roster.push([person.peerId,seat,person.build,choice.character.toString(),choice.costume.toString(),
        Array.from(choice.fighterPackage).join("."),Array.from(choice.costumePackage).join("."),choice.team].join(":"));
    }
  }
  if (roster.length < 2 || roster.length > 4 || rules.teams && !(teams & (teams-1))) return null;
  return roster.join("|");
}
