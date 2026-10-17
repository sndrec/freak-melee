export const MAX_MESSAGE_BYTES = 16 * 1024;
export const MAX_NAME_LENGTH = 32;
export const MAX_BUILD_LENGTH = 96;
export const MAX_PARTICIPANTS = 32;
export const MAX_ACTIVE_PLAYERS = 4;
export const MAX_SELECTION_BYTES = 512;
export const MAX_MATCH_BYTES = 4096;

export const enum RotationPolicy {
  Manual = 0,
  WinnerStays = 1,
  WinnerOut = 2,
}

export interface Participant {
  peerId: string;
  name: string;
  build: string;
  localPlayers: number;
  activeMask: number;
  waitingMask: number;
  ready: boolean;
  selection: Uint8Array;
  joinOrder: number;
  waitingRanks: [number, number, number, number];
}

export interface ConnectionAttachment extends Omit<Participant, "waitingRanks"> {
  connectionId: string;
  chatSubscribed?: boolean;
  chatLastPost?: number;
  chatHighestNonce?: number;
  chatLastHistory?: number;
  quickRevision?: number;
  quickDigest?: Uint8Array;
  waitingOrder: [number, number, number, number];
}

export interface RoomSnapshot {
  type: "presence";
  revision: number;
  ownerPeerId: string | null;
  participants: Participant[];
  matchEpoch: number;
  match: Uint8Array | null;
  rotationPolicy: RotationPolicy;
}

export function isPeerId(value: string): boolean {
  return /^[A-Za-z0-9_-]{8,64}$/.test(value);
}

export function isRoomId(value: string): boolean {
  return /^[A-Z0-9]{6,12}$/.test(value);
}

export function validSeatMasks(localPlayers: number, activeMask: number, waitingMask: number): boolean {
  const allSeats = (1 << localPlayers) - 1;
  return Number.isInteger(localPlayers) && localPlayers >= 0 && localPlayers <= 4 &&
    Number.isInteger(activeMask) && Number.isInteger(waitingMask) &&
    activeMask >= 0 && waitingMask >= 0 &&
    ((activeMask | waitingMask) & ~allSeats) === 0 &&
    (activeMask & waitingMask) === 0;
}

export function countActive(mask: number): number {
  let count = 0;
  for (let seats = mask; seats !== 0; seats &= seats - 1) ++count;
  return count;
}

export function isBoundedUtf8(
  value: string,
  minimum: number,
  maximum: number,
): boolean {
  const length = new TextEncoder().encode(value).byteLength;
  return length >= minimum && length <= maximum;
}

export function parseBoundedInteger(
  value: string | null,
  minimum: number,
  maximum: number,
): number | null {
  if (value === null || !/^[0-9]+$/.test(value)) {
    return null;
  }
  const parsed = Number(value);
  return Number.isSafeInteger(parsed) && parsed >= minimum && parsed <= maximum
    ? parsed
    : null;
}

export function publicParticipant(attachment: ConnectionAttachment): Participant {
  return {
    peerId: attachment.peerId,
    name: attachment.name,
    build: attachment.build,
    localPlayers: attachment.localPlayers,
    activeMask: attachment.activeMask,
    waitingMask: attachment.waitingMask,
    ready: attachment.ready,
    selection: attachment.selection.slice(),
    joinOrder: attachment.joinOrder,
    waitingRanks: [0, 0, 0, 0],
  };
}
