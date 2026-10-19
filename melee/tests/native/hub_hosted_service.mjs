// Exercises the hosted service with task-owned accounts and unlisted lobbies.
// It sends no public chat messages and keeps credentials out of files/logs.
import assert from 'node:assert/strict';
import { spawn } from 'node:child_process';
import { mkdir, open, writeFile, cp, rm } from 'node:fs/promises';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { createInterface } from 'node:readline';
import { accountKey } from '../../services/signaling/test/account-key.ts';
import { reviewGuestAccount } from './guest_account_hosted.mjs';

const root = resolve(dirname(fileURLToPath(import.meta.url)), '../../..');
const output = resolve(process.argv[2]);
const endpoint = 'https://freak-melee-signaling.sndrec32exe.workers.dev';
const fixture = resolve(root, '.local/native-room-rotation-2/base');
const cli = resolve(root, 'melee/build/native/native_account_test.exe');
const children = [], accounts = [], checks = [];
const guestAccountOnly = process.argv.includes('--guest-account-only');
let completed = false;
await mkdir(output, { recursive: false });
let nextRequest = 0;
async function call(person, route, body, token = person.grant?.token, expected = [200]) {
  // Leave room under the real per-network limit for native UI refreshes.
  await new Promise(resolve => setTimeout(resolve, Math.max(0, nextRequest - Date.now())));
  nextRequest = Date.now() + 1500;
  const response = await fetch(`${endpoint}/v1/accounts/${person.handle}/${route}`, {
    method: body === undefined ? 'GET' : 'POST',
    headers: { 'Content-Type': 'application/json', ...(token ? { Authorization: `Bearer ${token}` } : {}) },
    body: body === undefined ? undefined : JSON.stringify(body),
  });
  const result = await response.json();
  assert(expected.includes(response.status), `${route}: HTTP ${response.status}: ${result.error}`);
  return { ...result, httpStatus: response.status };
}
async function player(index) {
  const person = { handle: `zz_test_${crypto.randomUUID().replaceAll('-', '').slice(0, 13)}`, profile: resolve(output, `profile-${index}`) };
  const device = accountKey();
  const registration = await call(person, 'register/options', { name: `TEST Hub ${index}`, public_key: device.publicKey }, null);
  assert.equal(registration.origin, endpoint);
  person.grant = await call(person, 'register/finish', device.proof(person.handle, registration), null);
  person.key = device; accounts.push(person);

  await cp(fixture, person.profile, { recursive: true });
  const child = spawn(cli, ['--service', resolve(person.profile, 'Freak Melee/content'), endpoint],
    { windowsHide: true, stdio: ['pipe', 'pipe', 'pipe'] });
  children.push(child);
  let error = ''; child.stderr.on('data', data => { error += data; });
  const lines = createInterface({ input: child.stdout })[Symbol.asyncIterator]();
  const keyfile = resolve(output, `${person.handle}.fmakey`);
  await writeFile(keyfile, JSON.stringify({ application: 'Freak Melee account key', service: endpoint, account: person.handle, seed: device.seed() }));
  let timer;
  try {
    const line = await Promise.race([lines.next(), new Promise((_, reject) => {
      timer = setTimeout(() => reject(new Error(`Native saved-session validation timed out: ${error}`)), 35000);
    })]);
    assert(!line.done, `Native client exited: ${error}`); assert.match(line.value, /ready signed-in=0/);
    child.stdin.write(JSON.stringify({ operation: 'ImportKey', path: keyfile }) + '\n');
    const imported = await lines.next(); assert(!imported.done); assert.match(imported.value, /done signed-in=1/);
    await rm(keyfile);
  } finally { clearTimeout(timer); }
  const exited = new Promise(resolve => child.once('exit', resolve)); child.stdin.end();
  assert.equal(await exited, 0, `Native account driver failed: ${error}`);
  return person;
}
try {
  assert.equal((await fetch(`${endpoint}/health`)).status, 200);
  const a = await player('a'), b = await player('b');
  checks.push('Hosted registration options and signed proofs issue sessions accepted by the normal native HTTPS/DPAPI path');
  if (guestAccountOnly) {
    await reviewGuestAccount(endpoint, a, b, call, checks);
  } else {
    await call(a, 'social/friend', { to: b.handle, action: 'request' });
    await call(b, 'social/friend', { to: a.handle, action: 'accept' });
    for (const message of (await call(b, 'social/state', {})).messages)
      await call(b, 'social/delete', { id: message.id });
    const log = await open(resolve(output, 'ui.log'), 'w');
    const python = resolve(process.env.LOCALAPPDATA, 'Programs/Python/Python311/python.exe');
    const ui = spawn(python, [resolve(root, 'melee/tools/review_native_invitations.py'),
      '--exe', resolve(root, 'melee/build/native/melee_native.exe'), '--iso', resolve(root, 'Super Smash Bros. Melee (USA) (v1.02).iso'),
      '--profile-a', a.profile, '--profile-b', b.profile, '--a-handle', a.handle, '--b-handle', b.handle,
      '--endpoint', endpoint, '--unlisted', '--output', resolve(output, 'ui')],
      { windowsHide: true, stdio: ['ignore', log.fd, log.fd] });
    children.push(ui); const code = await new Promise(resolve => ui.once('exit', resolve)); await log.close();
    assert.equal(code, 0, 'Hosted native UI review failed; inspect ui.log');
    checks.push('Two hidden/muted native clients pass verified global/lobby profiles, offline invitations, lobby switching and sign-out against the hosted service');
  }
  completed = true;
} finally {
  for (const child of children) if (child.exitCode === null) child.kill();
  for (const person of accounts) {
    const challenge = await call(person, 'reauth/options', { public_key: person.key.publicKey }, person.grant.token, [200, 401]);
    if (challenge.httpStatus === 200) {
      await call(person, 'reauth/finish', person.key.proof(person.handle, challenge), person.grant.token);
      await call(person, 'sessions/revoke-all', {}, person.grant.token, [200, 401]);
    }
    await call(person, 'me', undefined, person.grant.token, [401]);
    for (const file of [resolve(output, `${person.handle}.fmakey`),
      resolve(person.profile, 'Freak Melee/content/account-session.bin'),
      resolve(output, 'ui', person.profile.endsWith('profile-a') ? 'twilight' : 'guest', 'appdata/Freak Melee/content/account-session.bin')])
      await rm(file, { force: true });
  }
}
assert(completed);
await writeFile(resolve(output, 'receipt.json'), JSON.stringify({ endpoint, accounts: accounts.map(person => person.handle), checks,
  limitations: [guestAccountOnly
    ? 'Native account CLI plus two WebSocket clients on one PC; the game-screen attachment check has separate local evidence.'
    : 'Two clients on one PC; this review does not establish multi-PC acceptance.'],
  cleanup: 'Native clients closed; both test sessions verified revoked. TEST profiles, public keys and test mailbox records remain; temporary key files and protected local test-key records were removed.' }, null, 2));
console.log(`${checks.length} hosted integration checks passed`);
