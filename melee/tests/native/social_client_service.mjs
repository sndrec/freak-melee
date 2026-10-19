// Full native social review with real service-issued sessions, isolated profiles,
// and hidden/muted game windows. No Windows passkey prompt or public messages.
import assert from 'node:assert/strict';
import { spawn } from 'node:child_process';
import { mkdir, open, writeFile, cp } from 'node:fs/promises';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { createInterface } from 'node:readline';
import { authenticator } from '../../services/signaling/test/account-authenticator.ts';

const root = resolve(dirname(fileURLToPath(import.meta.url)), '../../..');
const output = resolve(process.argv[2]);
const port = Number(process.argv[3] || 8800);
const endpoint = `http://localhost:${port}`;
const fixture = resolve(process.argv[4] || resolve(root, '.local/native-room-rotation-2/base'));
const serviceRoot = resolve(root, 'melee/services/signaling');
const cli = resolve(root, 'melee/build/native/native_account_test.exe');
await mkdir(output, { recursive: false });
const log = await open(resolve(output, 'service.log'), 'w');
const service = spawn(process.execPath, [resolve(serviceRoot, 'node_modules/wrangler/bin/wrangler.js'),
  'dev', '--local', '--ip', '127.0.0.1', '--port', String(port), '--persist-to', resolve(output, 'service-state'),
  '--var', `AUTH_ORIGIN:${endpoint}`, '--var', 'AUTH_RP_ID:localhost'],
  { cwd: serviceRoot, windowsHide: true, stdio: ['ignore', log.fd, log.fd] });
await writeFile(resolve(output, 'service-pid.txt'), String(service.pid));
const clients = [], checks = [];
async function call(player, route, body, token = player?.grant.token) {
  const response = await fetch(`${endpoint}/v1/accounts/${player.handle}/${route}`, {
    method: body === undefined ? 'GET' : 'POST',
    headers: { 'Content-Type': 'application/json', 'CF-Connecting-IP': 'social-fixture', ...(token ? { Authorization: `Bearer ${token}` } : {}) },
    body: body === undefined ? undefined : JSON.stringify(body),
  });
  const result = await response.json();
  assert.equal(response.status, 200, `${route}: ${result.error}`);
  return result;
}
async function player(handle, name, directory) {
  const person = { handle, profile: resolve(output, directory) };
  await cp(fixture, person.profile, { recursive: true });
  const device = await authenticator();
  person.registration = await call(person, 'register/options', { name }, null);
  person.grant = await call(person, 'register/finish', { attempt: person.registration.attempt,
    credential: await device.register(person.registration.options, person.registration.origin) }, null);
  person.authentication = await call(person, 'login/options', {}, null);
  const child = spawn(cli, ['--client', resolve(person.profile, 'Freak Melee/content'), endpoint],
    { windowsHide: true, stdio: ['pipe', 'pipe', 'pipe'] });
  clients.push(child);
  const lines = createInterface({ input: child.stdout })[Symbol.asyncIterator]();
  let error = ''; child.stderr.on('data', data => { error += data; });
  async function line() {
    let timer;
    try {
      const result = await Promise.race([lines.next(), new Promise((_, reject) => {
        timer = setTimeout(() => reject(new Error(`native social request timed out: ${error}`)), 35000);
      })]);
      assert(!result.done, `native client exited: ${error}`); return result.value;
    } finally { clearTimeout(timer); }
  }
  child.stdin.write(JSON.stringify({ handle, token: person.grant.token,
    registration: person.registration, authentication: person.authentication }) + '\n');
  assert.match(await line(), /ready signed-in=1/);
  person.send = async body => { child.stdin.write(JSON.stringify(body) + '\n'); return line(); };
  person.close = async () => {
    child.stdin.end(); const code = await new Promise(resolve => child.once('exit', resolve));
    assert.equal(code, 0, `native client failed: ${error}`);
  };
  return person;
}
try {
  for (let i = 0; ; ++i) {
    try { if ((await fetch(`http://127.0.0.1:${port}/health`)).ok) break; } catch {}
    assert(i < 100, 'local social service did not start');
    await new Promise(resolve => setTimeout(resolve, 200));
  }
  const a = await player('social_native_a', 'Twilight', 'profile-a');
  const b = await player('social_native_b', 'Guest', 'profile-b');
  assert.match(await a.send({ operation: 13 }), /social-loaded=1 mail=0 total=0 unread=0 friends=0/);
  assert.match(await a.send({ operation: 14, handle: b.handle }), /lookup=social_native_b relationship=none/);
  assert.match(await a.send({ operation: 18, handle: b.handle, action: 'request' }), /friends=1.*relationship=outgoing/);
  assert.match(await b.send({ operation: 13 }), /mail=1 total=1 unread=1 friends=1/);
  assert.match(await b.send({ operation: 18, handle: a.handle, action: 'accept' }), /relationship=friend/);
  checks.push('Native friend requests, account lookup and acceptance share persistent account identities');
  const nonce = Buffer.from(crypto.getRandomValues(new Uint8Array(32))).toString('base64url');
  const letter = { operation: 15, handle: b.handle, subject: 'From the native worker', text: 'Hello from the native worker.\nA second line.', nonce, folder: 1 };
  assert.match(await a.send(letter), /social-loaded=1 mail=1 total=1/);
  const state = await call(b, 'social/state', {});
  const mail = state.messages.find(message => message.title === letter.subject);
  assert(mail);
  assert.match(await b.send({ operation: 16, target: mail.id }), /opened=1/);
  assert.match(await b.send({ operation: 17, target: mail.id }), /total=1/);
  await a.send(letter);
  assert.equal((await call(b, 'social/state', {})).total, 1);
  checks.push('Native letter send/read/delete and retry preserve exactly one delivery');
  await a.close(); await b.close();
  // Leave one fresh incoming request for the actual in-game UI to accept.
  await call(a, 'social/friend', { to: b.handle, action: 'remove' });
  for (const message of (await call(b, 'social/state', {})).messages) await call(b, 'social/delete', { id: message.id });
  await call(a, 'social/friend', { to: b.handle, action: 'request' });
  const python = resolve(process.env.LOCALAPPDATA, 'Programs/Python/Python311/python.exe');
  const uiLog = await open(resolve(output, 'ui.log'), 'w');
  const ui = spawn(python, [resolve(root, 'melee/tools/review_native_social.py'),
    '--exe', resolve(root, 'melee/build/native/melee_native.exe'), '--iso', resolve(root, 'Super Smash Bros. Melee (USA) (v1.02).iso'),
    '--profile-a', a.profile, '--profile-b', b.profile, '--a-handle', a.handle, '--b-handle', b.handle,
    '--endpoint', endpoint, '--output', resolve(output, 'ui')],
    { windowsHide: true, stdio: ['ignore', uiLog.fd, uiLog.fd] });
  clients.push(ui);
  const code = await new Promise(resolve => ui.once('exit', resolve)); await uiLog.close();
  assert.equal(code, 0, 'native UI review failed; inspect its local ui.log');
  checks.push('Two signed-in hidden/muted game clients pass the native social UI review');
  await writeFile(resolve(output, 'receipt.json'), JSON.stringify({ checks, limitations: [
    'Software authenticator sessions exercise normal saved-session validation; real Windows Hello remains unverified.',
    'Chat/lobby identities and invitations remain separate integration work.'
  ] }, null, 2));
  console.log(`${checks.length} native social integration checks passed`);
} finally {
  for (const child of clients) if (child.exitCode === null) child.kill();
  if (service.exitCode === null) {
    const killer = spawn('taskkill.exe', ['/PID', String(service.pid), '/T', '/F'], { windowsHide: true, stdio: 'ignore' });
    await new Promise(resolve => killer.once('exit', resolve));
  }
  await log.close();
}
