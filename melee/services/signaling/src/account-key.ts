import { createPublicKey, verify } from "node:crypto";
import { Buffer } from "node:buffer";
import { AccountError, secretValue, type Ceremony } from "./account-protocol";

export interface AccountKeyChallenge {
  origin: string; handle: string; userId: string; purpose: Ceremony;
  publicKey: string; attempt: string; challenge: string;
}

// This exact payload is also constructed by the client. Every field is bounded
// and newline-free. Purpose and service binding keep proofs specific to this
// account operation, rather than exposing a general signing service.
export function accountKeyMessage(proof: AccountKeyChallenge): Uint8Array {
  return new TextEncoder().encode(["Freak Melee account key", proof.origin, proof.handle, proof.userId,
    proof.purpose, proof.publicKey, proof.attempt, proof.challenge].join("\n"));
}
export function verifyAccountKey(proof: AccountKeyChallenge, signature: unknown): boolean {
  secretValue(proof.publicKey);
  if (typeof signature !== "string" || !/^[A-Za-z0-9_-]{86}$/.test(signature))
    throw new AccountError(400, "Invalid account-key proof.");
  const bytes = Buffer.from(signature, "base64url");
  if (bytes.length !== 64 || bytes.toString("base64url") !== signature)
    throw new AccountError(400, "Invalid account-key proof.");
  const spki = Buffer.concat([Buffer.from("302a300506032b6570032100", "hex"), Buffer.from(proof.publicKey, "base64url")]);
  return verify(null, accountKeyMessage(proof), createPublicKey({ key: spki, format: "der", type: "spki" }), bytes);
}
