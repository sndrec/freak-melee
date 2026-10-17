import { isPeerId, isRoomId, isBoundedUtf8 } from "./protocol";

export type QuickMode = 1 | 2;
export const QUICK_SEARCH_LEASE_MS = 30_000;
export const QUICK_STAGES = ["battlefield", "final_destination", "fountain_of_dreams",
  "dream_land", "pokemon_stadium", "yoshis_story"] as const;
export const QUICK_RULES = { stocks: 4, minutes: 6, items: false } as const;
export interface QuickMember {
  peerId: string;
  name: string;
  localPlayers: number;
  connectionId: string;
}
export interface QuickOffer {
  partyId: string;
  roomCode: string;
  generation: number;
  mode: QuickMode;
  build: string;
  members: QuickMember[];
}
export interface QuickTicket extends QuickOffer {
  created: number;
  expires: number;
}
export interface QuickPlayer extends QuickMember {
  partyId: string;
  team: number;
  admission: string;
}
export interface QuickAllocation {
  id: string;
  roomCode: string;
  mode: QuickMode;
  build: string;
  created: number;
  stage: number;
  rules: typeof QUICK_RULES;
  parties: QuickTicket[];
  players: QuickPlayer[];
}
export interface QuickState {
  mode: number;
  state: number; // 0 idle, 1 searching/preparing, 2 assigned
  generation: number;
  matchRoom: string;
  admission: string;
  team: number;
  stage: number;
}
export function quickBucket(mode: QuickMode, build: string): string {
  return `${mode}:${build}`;
}
export function quickSeats(offer: QuickOffer): number {
  return offer.members.reduce((sum, member) => sum + member.localPlayers, 0);
}
export function validQuickOffer(offer: QuickOffer): boolean {
  if (!/^[a-f0-9]{64}$/.test(offer.partyId) || !isRoomId(offer.roomCode) ||
      !Number.isSafeInteger(offer.generation) || offer.generation <= 0 ||
      (offer.mode !== 1 && offer.mode !== 2) || !isBoundedUtf8(offer.build, 1, 96) ||
      offer.members.length < 1 || offer.members.length > 2) return false;
  const peers = new Set<string>();
  for (const member of offer.members) {
    if (!isPeerId(member.peerId) || peers.has(member.peerId) ||
        !isBoundedUtf8(member.name, 1, 32) ||
        !Number.isInteger(member.localPlayers) || member.localPlayers < 1 || member.localPlayers > 2 ||
        !/^[a-f0-9-]{36}$/.test(member.connectionId)) return false;
    peers.add(member.peerId);
  }
  return quickSeats(offer) <= offer.mode;
}
export function sameQuickMembers(a: QuickMember[], b: QuickMember[]): boolean {
  return a.length === b.length && a.every((member, index) =>
    member.peerId === b[index].peerId && member.localPlayers === b[index].localPlayers &&
    member.connectionId === b[index].connectionId);
}

// Prefer the oldest party that can form a complete match. A lone older solo
// does not prevent two ready pairs from matching each other.
export function chooseQuickTickets(tickets: QuickTicket[], mode: QuickMode): QuickTicket[] {
  if (mode === 1) return tickets.length >= 2 ? tickets.slice(0, 2) : [];
  const singles = tickets.filter((ticket) => quickSeats(ticket) === 1);
  const pairs = tickets.filter((ticket) => quickSeats(ticket) === 2);
  for (const first of tickets) {
    if (quickSeats(first) === 1) {
      const other = singles.filter((ticket) => ticket !== first);
      if (other.length && pairs.length) return [first, other[0], pairs[0]];
      if (other.length >= 3) return [first, ...other.slice(0, 3)];
    } else {
      const other = pairs.filter((ticket) => ticket !== first);
      if (other.length) return [first, other[0]];
      if (singles.length >= 2) return [first, ...singles.slice(0, 2)];
    }
  }
  return [];
}
export function allocateQuickMatch(parties: QuickTicket[]): QuickAllocation {
  const mode = parties[0].mode;
  if (parties.length < 2 || parties.some((party) => party.mode !== mode || party.build !== parties[0].build) ||
      parties.reduce((sum, party) => sum + quickSeats(party), 0) !== mode * 2)
    throw new Error("Invalid matchmaking assembly");
  const capacity = [mode, mode];
  const players: QuickPlayer[] = [];
  // Fill whole premade sides first, then place solo players in the vacancies.
  for (const party of [...parties].sort((a, b) => quickSeats(b) - quickSeats(a))) {
    const seats = quickSeats(party);
    const team = capacity[0] >= seats ? 0 : 1;
    if (capacity[team] < seats) throw new Error("Premade cannot fit its team");
    capacity[team] -= seats;
    for (const member of party.members) players.push({ ...member, partyId: party.partyId, team,
      admission: crypto.randomUUID().replaceAll("-", "") });
  }
  const random = crypto.getRandomValues(new Uint32Array(1))[0];
  return { id: crypto.randomUUID(), roomCode: crypto.randomUUID().replaceAll("-", "").slice(0, 12).toUpperCase(),
    mode, build: parties[0].build, created: Date.now(), stage: random % QUICK_STAGES.length,
    rules: QUICK_RULES, parties, players };
}
