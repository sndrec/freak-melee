export interface LobbyRules {
  mode: number; time: number; stocks: number; handicap: number; damage: number;
  stagePolicy: number; stockTime: number; friendlyFire: number; pause: number;
  score: number; sdPenalty: number; itemFrequency: number; teams: number;
  itemMask: bigint; stageMask: number;
}

export interface LobbyState {
  revision: number; phase: number; remainingMs: number; rules: LobbyRules | null;
}

export const LOBBY_RULES_BYTES = 25;
export const LOBBY_COUNTDOWN_MS = 3000;

export function decodeLobbyRules(bytes: Uint8Array): LobbyRules {
  if (bytes.length !== LOBBY_RULES_BYTES) throw new Error("Invalid lobby rules size");
  const limits = [2, 99, 99, 2, 20, 4, 99, 1, 1, 1, 2, 4, 1];
  for (let i = 0; i < limits.length; ++i)
    if (bytes[i] > limits[i] && !(i === 11 && bytes[i] === 255)) throw new Error("Invalid lobby rule");
  if (!bytes[2] || bytes[4]<5 || bytes[0]!==1 && !bytes[1]) throw new Error("Invalid stock count, time or damage ratio");
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  return { mode: bytes[0], time: bytes[1], stocks: bytes[2], handicap: bytes[3], damage: bytes[4],
    stagePolicy: bytes[5], stockTime: bytes[6], friendlyFire: bytes[7], pause: bytes[8], score: bytes[9],
    sdPenalty: bytes[10], itemFrequency: bytes[11], teams: bytes[12], itemMask: view.getBigUint64(13), stageMask: view.getUint32(21) };
}

export function encodeLobbyRules(rules: LobbyRules): Uint8Array {
  const bytes = new Uint8Array(LOBBY_RULES_BYTES);
  bytes.set([rules.mode, rules.time, rules.stocks, rules.handicap, rules.damage, rules.stagePolicy,
    rules.stockTime, rules.friendlyFire, rules.pause, rules.score, rules.sdPenalty, rules.itemFrequency, rules.teams]);
  const view = new DataView(bytes.buffer);
  view.setBigUint64(13, rules.itemMask); view.setUint32(21, rules.stageMask);
  decodeLobbyRules(bytes);
  return bytes;
}
