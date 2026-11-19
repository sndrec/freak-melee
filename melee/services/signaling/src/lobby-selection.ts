export interface FighterChoice {
  character: bigint; costume: bigint; fighterPackage: Uint8Array; costumePackage: Uint8Array;
  team: number; selected: boolean;
}
const EMPTY_PACKAGE = new Uint8Array(32);

export function unreadySelection(selection: Uint8Array): Uint8Array {
  const result = selection.slice();
  if (result.length >= 7 && new DataView(result.buffer).getUint32(0) === 0x464d534c && result[4] === 1)
    result[6] &= ~3;
  return result;
}

// Decode the complete FMSL record before accepting readiness. Inactive local
// seats may be empty; every fighting seat must have a selected fighter.
export function lockedChoices(bytes: Uint8Array, players: number,
  activeMask = (1 << players) - 1): FighterChoice[] | null {
  if (bytes[6] !== 1 && bytes[6] !== 3) return null;
  return decodeChoices(bytes, players, activeMask);
}

export function selectedSeats(bytes: Uint8Array, players: number): number {
  return decodeChoices(bytes, players, 0)?.reduce((mask, choice, seat) =>
    choice.selected ? mask | (1 << seat) : mask, 0) ?? 0;
}

function decodeChoices(bytes: Uint8Array, players: number,
  required: number): FighterChoice[] | null {
  if (players < 1 || players > 4 || required & ~((1 << players) - 1) || bytes.length < 7 || bytes.length > 335) return null;
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  if (view.getUint32(0) !== 0x464d534c || bytes[4] !== 1 || bytes[5] !== players ||
      (bytes[6] > 3 || bytes[6] === 2)) return null;
  let offset = 7;
  const choices: FighterChoice[] = [];
  for (let seat = 0; seat < players; ++seat) {
    if (offset + 18 > bytes.length) return null;
    const flags = bytes[offset++], team = bytes[offset++];
    if ((required & (1 << seat)) && !(flags & 1) || flags & ~7 || team > 2) return null;
    const character = view.getBigUint64(offset); offset += 8;
    const costume = view.getBigUint64(offset); offset += 8;
    const packages: Uint8Array[] = [];
    for (const flag of [2, 4]) {
      if (!(flags & flag)) { packages.push(EMPTY_PACKAGE); continue; }
      if (offset + 32 > bytes.length) return null;
      const id = bytes.slice(offset, offset + 32); offset += 32;
      if (!id.some((byte) => byte !== 0)) return null;
      packages.push(id);
    }
    if (flags & 1) {
      if (flags & 2 ? character !== 0n || costume !== 0n : character === 0n || costume === 0n) return null;
    } else if (flags || character || costume) return null;
    choices.push({ character, costume, fighterPackage: packages[0], costumePackage: packages[1], team, selected: !!(flags & 1) });
  }
  return offset === bytes.length ? choices : null;
}
