import { encodeCBOR, type CBORType } from "@levischuck/tiny-cbor";
import { isoBase64URL } from "@simplewebauthn/server/helpers";
import type { RegistrationResponseJSON, AuthenticationResponseJSON,
  PublicKeyCredentialCreationOptionsJSON, PublicKeyCredentialRequestOptionsJSON } from "@simplewebauthn/server";

export function concat(...parts: Uint8Array[]): Uint8Array<ArrayBuffer> {
  const output = new Uint8Array(parts.reduce((n,part) => n + part.length, 0));
  let offset = 0;
  for (const part of parts) { output.set(part, offset); offset += part.length; }
  return output;
}
function derInteger(bytes: Uint8Array): Uint8Array {
  let start = 0;
  while (start < bytes.length - 1 && !bytes[start]) ++start;
  const value = bytes.subarray(start);
  const pad = value[0] & 128 ? 1 : 0;
  return concat(new Uint8Array([2, value.length + pad]), new Uint8Array(pad), value);
}

// A software authenticator used only in tests. Responses go through the same
// production WebAuthn verification functions as Windows-generated responses.
export async function authenticator() {
  const keys = await crypto.subtle.generateKey({ name: "ECDSA", namedCurve: "P-256" }, true, ["sign", "verify"]);
  if (!("publicKey" in keys)) throw new Error("ECDSA key pair expected");
  const publicKey = await crypto.subtle.exportKey("jwk", keys.publicKey);
  if (!("x" in publicKey) || !("y" in publicKey)) throw new Error("EC public key expected");
  const id = crypto.getRandomValues(new Uint8Array(32));
  const idString = isoBase64URL.fromBuffer(id);
  let counter = 0, userId = "";

  async function authData(rp: string, flags: number, count: number): Promise<Uint8Array<ArrayBuffer>> {
    const tail = new Uint8Array(5);
    tail[0] = flags; new DataView(tail.buffer).setUint32(1, count);
    return concat(new Uint8Array(await crypto.subtle.digest("SHA-256", new TextEncoder().encode(rp))), tail);
  }
  return {
    id: idString,
    async register(options: PublicKeyCredentialCreationOptionsJSON, origin: string,
      flags = 0x45, rp = options.rp.id!): Promise<RegistrationResponseJSON> {
      userId = options.user.id;
      const cose = encodeCBOR(new Map<number, CBORType>([[1, 2], [3, -7], [-1, 1],
        [-2, isoBase64URL.toBuffer(publicKey.x!)], [-3, isoBase64URL.toBuffer(publicKey.y!)]]));
      const length = new Uint8Array(2); new DataView(length.buffer).setUint16(0, id.length);
      const data = concat(await authData(rp, flags, 0), new Uint8Array(16), length, id, cose);
      const attestation = encodeCBOR(new Map<string, CBORType>([["fmt", "none"], ["authData", data], ["attStmt", new Map()]]));
      return { id: idString, rawId: idString, type: "public-key", clientExtensionResults: {}, response: {
        clientDataJSON: isoBase64URL.fromUTF8String(JSON.stringify({ type: "webauthn.create", challenge: options.challenge, origin, crossOrigin: false })),
        attestationObject: isoBase64URL.fromBuffer(new Uint8Array(attestation)),
      } };
    },
    async sign(options: PublicKeyCredentialRequestOptionsJSON, origin: string,
      flags = 0x05, rp = options.rpId!, count?: number): Promise<AuthenticationResponseJSON> {
      const data = await authData(rp, flags, count ?? ++counter);
      const clientData = JSON.stringify({ type: "webauthn.get", challenge: options.challenge, origin, crossOrigin: false });
      const hash = new Uint8Array(await crypto.subtle.digest("SHA-256", new TextEncoder().encode(clientData)));
      const signature = new Uint8Array(await crypto.subtle.sign({ name: "ECDSA", hash: "SHA-256" }, keys.privateKey, concat(data, hash)));
      const integers = concat(derInteger(signature.subarray(0,32)), derInteger(signature.subarray(32)));
      const der = concat(new Uint8Array([0x30, integers.length]), integers);
      return { id: idString, rawId: idString, type: "public-key", clientExtensionResults: {}, response: {
        clientDataJSON: isoBase64URL.fromUTF8String(clientData), authenticatorData: isoBase64URL.fromBuffer(data),
        signature: isoBase64URL.fromBuffer(der), userHandle: userId,
      } };
    },
  };
}
