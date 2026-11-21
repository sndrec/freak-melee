import type { QuickOffer } from "./quick-play";
import { equalHash } from "./account-protocol";

// This property is stored in the source party only. The queue receives a plain QuickOffer.
export interface StoredQuickRoster extends QuickOffer {
  owners?: string[];
}

export function ownsPartySeat(roster: StoredQuickRoster, peerId: string, localPlayers: number,
  build: string, identity: string, reconnectId: string, boundAccount: string | undefined): boolean {
  if (roster.build !== build) return false;
  const index = roster.members.findIndex(member => member.peerId === peerId && member.localPlayers === localPlayers);
  if (index < 0) return false;
  const connection = roster.members[index].connectionId;
  return roster.owners?.[index] === identity || (boundAccount !== undefined && identity === `account:${boundAccount}`) ||
    (reconnectId.length === connection.length && equalHash(reconnectId, connection));
}

export function pruneRoomAccounts(ctx: DurableObjectState, joining: Set<string>): void {
  const live = new Set(joining);
  for (const socket of ctx.getWebSockets()) {
    if (socket.readyState !== WebSocket.OPEN) continue;
    const peer = socket.deserializeAttachment();
    if (typeof peer?.peerId === "string" && !peer.superseded) live.add(peer.peerId);
  }
  ctx.storage.sql.exec(`
    DELETE FROM room_accounts
    WHERE peer NOT IN (SELECT value FROM json_each(?))
      AND peer NOT IN (SELECT peer FROM room_chat)
      AND peer NOT IN (SELECT loser_peer FROM room_stage)
      AND NOT EXISTS (SELECT 1 FROM room_recovery WHERE expires>? AND json_extract(attachment,'$.peerId')=room_accounts.peer)
      AND NOT EXISTS (SELECT 1 FROM lobby_flow WHERE room_accounts.peer IN
        (json_extract(history,'$.turn.peerId'),json_extract(next_history,'$.turn.peerId')))
      AND NOT EXISTS (SELECT 1 FROM quick_party, json_each(roster,'$.members') AS member
        WHERE state IN (1,2,3) AND json_extract(member.value,'$.peerId')=room_accounts.peer)
      AND NOT EXISTS (SELECT 1 FROM quick_match, json_each(allocation,'$.players') AS player
        WHERE aborted=0 AND json_extract(player.value,'$.peerId')=room_accounts.peer)
  `, JSON.stringify([...live]), Date.now());
}
