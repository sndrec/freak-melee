import { generateKeyPairSync, sign } from "node:crypto";
import { Buffer } from "node:buffer";

export interface KeyChallenge {
  attempt: string; challenge: string; user_id: string; public_key: string;
  purpose: string; origin: string;
}
export function accountKey() {
  const pair = generateKeyPairSync("ed25519");
  const publicKey = pair.publicKey.export({ format: "der", type: "spki" }).subarray(-32).toString("base64url");
  return {
    publicKey,
    seed: () => pair.privateKey.export({ format: "der", type: "pkcs8" }).subarray(-32).toString("base64url"),
    proof(handle: string, challenge: KeyChallenge) {
      const message = Buffer.from(["Freak Melee account key", challenge.origin, handle, challenge.user_id,
        challenge.purpose, challenge.public_key, challenge.attempt, challenge.challenge].join("\n"));
      return { attempt: challenge.attempt, signature: sign(null, message, pair.privateKey).toString("base64url") };
    },
  };
}
