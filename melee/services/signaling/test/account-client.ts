import { env, exports } from "cloudflare:workers";
import { expect } from "vitest";
import type { PublicKeyCredentialCreationOptionsJSON } from "@simplewebauthn/server";
import { authenticator } from "./account-authenticator";
import type { SocialProfile, SocialResult } from "../src/social-protocol";

export async function player() {
  const handle = "social_" + crypto.randomUUID().replaceAll("-", "").slice(0, 17);
  const headers = { "Content-Type": "application/json", "CF-Connecting-IP": crypto.randomUUID() };
  async function request(path: string, body?: unknown, token?: string) {
    return exports.default.fetch(new Request(`https://signal.test/v1/accounts/${handle}/${path}`, {
      method: body === undefined ? "GET" : "POST", headers: { ...headers, ...(token ? { Authorization: `Bearer ${token}` } : {}) },
      body: body === undefined ? undefined : JSON.stringify(body),
    }), env);
  }
  const device = await authenticator();
  const start = await (await request("register/options", { name: handle })).json<{
    attempt: string; origin: string; options: PublicKeyCredentialCreationOptionsJSON }>();
  const response = await request("register/finish", { attempt: start.attempt,
    credential: await device.register(start.options, start.origin) });
  expect(response.status).toBe(200);
  const grant = await response.json<{ profile: SocialProfile; token: string }>();
  const mailbox = env.SOCIAL.getByName(grant.profile.id);
  async function social(route: string, body: unknown = {}) { return request(`social/${route}`, body, grant.token); }
  async function state(folder = 0) {
    const response = await social("state", { folder }); expect(response.status).toBe(200);
    return response.json<SocialResult>();
  }
  return { handle, request, grant, social, state, mailbox };
}
