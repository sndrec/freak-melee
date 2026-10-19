import type { Participant } from "./protocol";

export interface LobbyStagePlayer { peerId: string; seat: number }

// Match ports use join order, then the local seat number, even on shared PCs.
export function lobbyStagePlayers(people: Participant[]): LobbyStagePlayer[] {
  return [...people].sort((a, b) => a.joinOrder - b.joinOrder).flatMap((person) =>
    Array.from({ length: person.localPlayers }, (_, seat) => ({ peerId: person.peerId, seat }))
      .filter((player) => (person.activeMask & (1 << player.seat)) !== 0));
}

export function lobbyStageChooser(people: Participant[], loser: LobbyStagePlayer): LobbyStagePlayer | null {
  const players = lobbyStagePlayers(people);
  return players.find((player) => player.peerId === loser.peerId && player.seat === loser.seat)
    ?? players[0] ?? null;
}
