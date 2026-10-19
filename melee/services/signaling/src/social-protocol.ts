import { AccountError, accountHandle, secretValue, text } from "./account-protocol";

export interface SocialProfile { id: string; handle: string; name: string; bio: string }
export interface SocialMailSummary {
  id: string; folder: number; kind: string; peer: string; handle: string; name: string;
  title: string; created: number; unread: number; status: string;
  room: string; expires: number;
}
export interface SocialFriendEntry { peer: string; handle: string; name: string; state: string; revision: number }
export interface SocialResult {
  error?: string; status?: number | string; ok?: boolean; id?: string;
  folder?: number; offset?: number; friends_offset?: number; total?: number; unread?: number; friends_total?: number;
  messages?: SocialMailSummary[]; friends?: SocialFriendEntry[];
  message?: SocialMailSummary & { body: string };
  profile?: SocialProfile; blocked?: boolean; relationship?: string;
  room?: string;
}
export type FriendState = "none" | "incoming" | "outgoing" | "friend";
export interface FriendUpdate {
  owner: SocialProfile; peer: SocialProfile; pair: string; revision: number;
  state: FriendState; notice: string; created: number;
}
export interface MailDelivery {
  id: string; sender: SocialProfile; recipient: SocialProfile;
  title: string; body: string; created: number;
  invitation?: { room: string; expires: number };
}
export const SOCIAL_PAGE = 8, SOCIAL_FRIENDS = 256, SOCIAL_MAIL = 2048;
export const DELIVERY_MS = 7 * 24 * 60 * 60_000;
export const RECEIPT_MS = 30 * 24 * 60 * 60_000;
export function socialOffset(value: unknown, maximum: number): number {
  if (value === undefined) return 0;
  if (typeof value !== "number" || !Number.isSafeInteger(value) || value < 0 || value > maximum)
    throw new AccountError(400, "Invalid page number.");
  return value;
}
export function socialHandle(value: unknown): string {
  const result = accountHandle(text(value, 24));
  if (!result) throw new AccountError(400, "Enter an account name using 3-24 letters, numbers or underscores.");
  return result;
}
export function pairName(first: string, second: string): string {
  secretValue(first); secretValue(second);
  if (first === second) throw new AccountError(400, "Choose another player.");
  return first < second ? `${first}:${second}` : `${second}:${first}`;
}
export function requireOwner(owner: SocialProfile, expected: string | undefined): void {
  if (secretValue(owner.id) !== expected) throw new AccountError(403, "Account identity does not match this mailbox.");
}
export function messageId(value: unknown): string { return secretValue(value); }
