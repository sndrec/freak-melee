import type { Participant } from "./protocol";
import type { QuickAllocation } from "./quick-play";

export interface QuickPort { peerId: string; seat: number; team: number }
export interface QuickStageVote { peer: string; seat: number; stage: number }

export function quickPorts(allocation: QuickAllocation, people: Participant[]): QuickPort[] {
  return people.flatMap((person) => {
    const assigned = allocation.players.find((p) => p.peerId === person.peerId);
    if (!assigned || assigned.localPlayers !== person.localPlayers) throw new Error("Quick player assignment changed");
    return Array.from({ length: person.localPlayers }, (_, seat) => ({ peerId: person.peerId, seat, team: assigned.team }));
  });
}

export function quickStageVotes(ports: QuickPort[], votes: QuickStageVote[]): number[] {
  return Array.from({ length: 4 }, (_, port) => {
    const player = ports[port];
    return player ? votes.find((v) => v.peer === player.peerId && v.seat === player.seat)?.stage ?? 255 : 255;
  });
}

export function agreedQuickStage(ports: QuickPort[], chooser: number, votes: QuickStageVote[]): number | null {
  const choosing = ports.filter((port) => port.team === chooser);
  if (!choosing.length) return null;
  let stage: number | undefined;
  for (const port of choosing) {
    const vote = votes.find((v) => v.peer === port.peerId && v.seat === port.seat);
    if (!vote || vote.stage > 5 || (stage !== undefined && vote.stage !== stage)) return null;
    stage = vote.stage;
  }
  return stage ?? null;
}
