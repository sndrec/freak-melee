import { AccountError, accountJson, secretHash, secretValue, text } from "./account-protocol";
import { pairName, socialHandle, type SocialProfile } from "./social-protocol";

export const FIGHTER_GIFT_TTL_MS = 7 * 24 * 60 * 60_000;
const MAX_PACKAGE_BYTES = 12 * 1024 * 1024;

export function fighterGiftKey(owner: string, id: string, upload: string): string {
  const prefix = `fighters/${owner}/${id}`;
  return upload ? `${prefix}/${upload}` : prefix;
}

export async function fighterGiftRequest(request: Request, env: CloudflareBindings,
  actor: SocialProfile, path: string): Promise<Response> {
  const mailbox = env.SOCIAL.getByName(actor.id);
  const parts = path.split("/");
  if (request.method === "GET" && parts.length === 2) {
    const id = secretValue(parts[1]);
    const gift = await mailbox.fighterGift(actor, id);
    if (!gift) throw new AccountError(404, "This fighter is no longer available.");
    const object = await env.FIGHTER_GIFTS.get(gift.key);
    if (!object || object.customMetadata?.hash !== gift.hash)
      throw new AccountError(410, "This fighter is no longer available.");
    return new Response(object.body, { headers: {
      "Content-Type": "application/octet-stream", "Content-Length": String(object.size),
      "Cache-Control": "no-store", "X-Content-Type-Options": "nosniff",
    } });
  }
  if (request.method !== "POST" || parts.length !== 4)
    throw new AccountError(404, "Unknown fighter delivery request.");
  const recipient = await env.ACCOUNTS.getByName(socialHandle(parts[1])).lookupProfile();
  if (!recipient || recipient.id === actor.id) throw new AccountError(404, "Choose a friend to receive the fighter.");
  const nonce = secretValue(parts[2]), hash = parts[3];
  const title = text(new URL(request.url).searchParams.get("title"), 63);
  if (!title || !/^[0-9a-f]{64}$/.test(hash)) throw new AccountError(400, "Invalid fighter details.");
  const body = request.body;
  const size = Number(request.headers.get("Content-Length"));
  if (!body || !Number.isInteger(size) || size < 208 || size > MAX_PACKAGE_BYTES)
    throw new AccountError(413, "Fighter packages must fit within 12 MiB.");
  if (request.headers.get("Content-Type") !== "application/octet-stream")
    throw new AccountError(415, "Expected a fighter package.");
  const target = env.SOCIAL.getByName(recipient.id);
  const id = secretHash(`${actor.id}:${nonce}`);
  const friendship = env.FRIENDSHIPS.getByName(pairName(actor.id, recipient.id));
  if (!await friendship.accepted(actor.id, recipient.id))
    throw new AccountError(403, "Only friends can send each other fighters.");
  const status = await target.fighterDeliveryStatus(recipient, actor.id, id, hash);
  if (!status)
    throw new AccountError(403, "Only friends with room in their inbox can receive fighters.");
  if (!await mailbox.reserveFighterSend(actor, recipient.id, id, hash, status === "ready"))
    throw new AccountError(429, "Daily fighter delivery limit reached.");
  if (status !== "ready") return accountJson({ id });
  // Each attempt owns its bytes until the mailbox adopts one of the uploads.
  const upload = crypto.randomUUID();
  const key = fighterGiftKey(recipient.id, id, upload);
  const object = await env.FIGHTER_GIFTS.put(key, body, {
    sha256: hash, customMetadata: { hash },
  });
  if (object.size !== size) {
    await env.FIGHTER_GIFTS.delete(key);
    throw new AccountError(400, "The fighter package size changed during upload.");
  }
  const received = await target.receiveFighter(recipient, actor, id, title, hash, upload);
  // An interrupted RPC can already have committed. Only discard a known loser.
  if (received !== "stored") await env.FIGHTER_GIFTS.delete(key);
  if (!received) throw new AccountError(409, "This friend cannot receive another fighter right now.");
  return accountJson({ id });
}
