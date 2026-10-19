// Full native social review with real service-issued sessions, isolated profiles,
// and hidden/muted game windows. No OS authentication prompts or public messages.
import assert from 'node:assert/strict';
import { spawn } from 'node:child_process';
import { mkdir, open, writeFile, cp, rm } from 'node:fs/promises';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { createInterface } from 'node:readline';
import { accountKey } from '../../services/signaling/test/account-key.ts';

const root = resolve(dirname(fileURLToPath(import.meta.url)), '../../..');
const output = resolve(process.argv[2]);
const port = Number(process.argv[3] || 8800);
const endpoint = `http://localhost:${port}`;
const fixture = resolve(process.argv[4] || resolve(root, '.local/native-room-rotation-2/base'));
const serviceRoot = resolve(root, 'melee/services/signaling');
const cli = resolve(root, 'melee/build/native/native_account_test.exe');
const guestAccountOnly = process.argv.includes('--guest-account-only');
await mkdir(output, { recursive: false });
const log = await open(resolve(output, 'service.log'), 'w');
const service = spawn(process.execPath, [resolve(serviceRoot, 'node_modules/wrangler/bin/wrangler.js'),
  'dev', '--local', '--ip', '127.0.0.1', '--port', String(port), '--persist-to', resolve(output, 'service-state'),
  '--var', `AUTH_ORIGIN:${endpoint}`],
  { cwd: serviceRoot, windowsHide: true, stdio: ['ignore', log.fd, log.fd] });
await writeFile(resolve(output, 'service-pid.txt'), String(service.pid));
const clients = [], checks = [];
let lastRequest = 0;
async function call(player, route, body, token = player?.grant.token) {
  // Keep fixture setup below the real service's 60 requests/minute limit.
  await new Promise(resolve => setTimeout(resolve, Math.max(0, 1100 - (Date.now() - lastRequest))));
  lastRequest = Date.now();
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
  const device = accountKey();
  person.registration = await call(person, 'register/options', { name, public_key: device.publicKey }, null);
  person.grant = await call(person, 'register/finish', device.proof(handle, person.registration), null);
  const keyfile = resolve(output, `${handle}.fmakey`);
  await writeFile(keyfile, JSON.stringify({ application: 'Freak Melee account key', service: endpoint, account: handle, seed: device.seed() }));
  const child = spawn(cli, ['--service', resolve(person.profile, 'Freak Melee/content'), endpoint],
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
  assert.match(await line(), /ready signed-in=0/);
  child.stdin.write(JSON.stringify({ operation: 'ImportKey', path: keyfile }) + '\n');
  assert.match(await line(), /done signed-in=1/);
  await rm(keyfile);
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
  assert.match(await a.send({ operation: 'RefreshSocial' }), /social-loaded=1 mail=0 total=0 unread=0 friends=0/);
  assert.match(await a.send({ operation: 'LookupProfile', handle: b.handle }), /lookup=social_native_b relationship=none/);
  assert.match(await a.send({ operation: 'ChangeFriend', handle: b.handle, action: 'request' }), /friends=1.*relationship=outgoing/);
  assert.match(await b.send({ operation: 'RefreshSocial' }), /mail=1 total=1 unread=1 friends=1/);
  assert.match(await b.send({ operation: 'ChangeFriend', handle: a.handle, action: 'accept' }), /relationship=friend/);
  checks.push('Native friend requests, account lookup and acceptance share persistent account identities');
  const nonce = Buffer.from(crypto.getRandomValues(new Uint8Array(32))).toString('base64url');
  const letter = { operation: 'SendLetter', handle: b.handle, subject: 'From the native worker', text: 'Hello from the native worker.\nA second line.', nonce, folder: 1 };
  assert.match(await a.send(letter), /social-loaded=1 mail=1 total=1/);
  const state = await call(b, 'social/state', {});
  const mail = state.messages.find(message => message.title === letter.subject);
  assert(mail);
  assert.match(await b.send({ operation: 'ReadLetter', target: mail.id }), /opened=1/);
  assert.match(await b.send({ operation: 'DeleteLetter', target: mail.id }), /total=1/);
  await a.send(letter);
  assert.equal((await call(b, 'social/state', {})).total, 1);
  checks.push('Native letter send/read/delete and retry preserve exactly one delivery');
  await a.close(); await b.close();
  if (!guestAccountOnly) {
    // Leave one fresh incoming request for the actual in-game UI to accept.
    await call(a, 'social/friend', { to: b.handle, action: 'remove' });
    for (const message of (await call(b, 'social/state', {})).messages) await call(b, 'social/delete', { id: message.id });
    await call(a, 'social/friend', { to: b.handle, action: 'request' });
  }
  const python = resolve(process.env.LOCALAPPDATA, 'Programs/Python/Python311/python.exe');
  const uiLog = await open(resolve(output, 'ui.log'), 'w');
  const ui = spawn(python, [resolve(root, guestAccountOnly ? 'melee/tools/review_native_guest_account.py' : 'melee/tools/review_native_social.py'),
    '--exe', resolve(root, 'melee/build/native/melee_native.exe'), '--iso', resolve(root, 'Super Smash Bros. Melee (USA) (v1.02).iso'),
    '--profile-a', a.profile, '--profile-b', b.profile, '--a-handle', a.handle, '--b-handle', b.handle,
    '--endpoint', endpoint, '--output', resolve(output, 'ui')],
    { windowsHide: true, stdio: ['ignore', uiLog.fd, uiLog.fd] });
  clients.push(ui);
  const code = await new Promise(resolve => ui.once('exit', resolve)); await uiLog.close();
  assert.equal(code, 0, 'native UI review failed; inspect its local ui.log');
  checks.push(guestAccountOnly ? 'A guest host signs in without leaving or changing seats, sends an invitation and leaves on sign-out'
    : 'Two signed-in hidden/muted game clients pass the native social UI review');
  if (!guestAccountOnly) {
    console.log('Social pages passed; waiting for the native request-rate window before the invitation review.');
    await new Promise(resolve => setTimeout(resolve, 61000));
    // Give the invitation review accepted friends with empty mailboxes.
    await call(a, 'social/friend', { to: b.handle, action: 'request' });
    await call(b, 'social/friend', { to: a.handle, action: 'accept' });
    for (const person of [a, b]) for (const folder of [0, 1])
      for (const message of (await call(person, 'social/state', { folder })).messages)
        await call(person, 'social/delete', { id: message.id });
    const invitationLog = await open(resolve(output, 'invitations.log'), 'w');
    const invitationReview = spawn(python, [resolve(root, 'melee/tools/review_native_invitations.py'),
      '--exe', resolve(root, 'melee/build/native/melee_native.exe'), '--iso', resolve(root, 'Super Smash Bros. Melee (USA) (v1.02).iso'),
      '--profile-a', a.profile, '--profile-b', b.profile, '--a-handle', a.handle, '--b-handle', b.handle,
      '--endpoint', endpoint, '--output', resolve(output, 'invitations')],
      { windowsHide: true, stdio: ['ignore', invitationLog.fd, invitationLog.fd] });
    clients.push(invitationReview);
    const invitationCode = await new Promise(resolve => invitationReview.once('exit', resolve)); await invitationLog.close();
    assert.equal(invitationCode, 0, 'native invitation review failed; inspect invitations.log');
    checks.push('Verified global/lobby profiles, lobby invitations, joining and account sign-out pass in two native clients');
    // Exercise a populated second page, including deletion of its final row.
    // These additional fixture keys stay in memory and never enter a player store.
    for (let i = 0; i < 8; ++i) {
      const person = { handle: `paging_friend_${i}` }, key = accountKey();
      const options = await call(person, 'register/options', { name: `Paging friend ${i}`, public_key: key.publicKey }, null);
      person.grant = await call(person, 'register/finish', key.proof(person.handle, options), null);
      await call(person, 'social/friend', { to: b.handle, action: 'request' });
      await call(b, 'social/friend', { to: person.handle, action: 'accept' });
    }
    for (const folder of [0, 1]) {
      for (;;) {
        const page = (await call(b, 'social/state', { folder })).messages;
        if (!page.length) break;
        for (const message of page) await call(b, 'social/delete', { id: message.id });
      }
    }
    for (let i = 0; i < 9; ++i) await call(a, 'social/send', {
      to: b.handle, title: `Paging letter ${i}`, body: `Fixture letter ${i}.`,
      nonce: Buffer.from(crypto.getRandomValues(new Uint8Array(32))).toString('base64url'),
    });
    console.log('Invitations passed; waiting for the native request-rate window before the paging review.');
    await new Promise(resolve => setTimeout(resolve, 61000));
    const pagingLog = await open(resolve(output, 'paging.log'), 'w');
    const pagingReview = spawn(python, [resolve(root, 'melee/tools/review_native_social_paging.py'),
      '--exe', resolve(root, 'melee/build/native/melee_native.exe'), '--iso', resolve(root, 'Super Smash Bros. Melee (USA) (v1.02).iso'),
      '--profile', b.profile, '--lookup-handle', a.handle, '--endpoint', endpoint, '--output', resolve(output, 'paging')],
      { windowsHide: true, stdio: ['ignore', pagingLog.fd, pagingLog.fd] });
    clients.push(pagingReview);
    const pagingCode = await new Promise(resolve => pagingReview.once('exit', resolve)); await pagingLog.close();
    assert.equal(pagingCode, 0, 'native social paging review failed; inspect paging.log');
    checks.push('Background refresh preserves typing; deleting the last row recovers Friends and Inbox pagination');
  }
  await writeFile(resolve(output, 'receipt.json'), JSON.stringify({ checks, limitations: [
    'Ed25519 key import and native signatures exercise the normal saved-session path.',
    'The service remains local to this review; no production deployment or multi-PC acceptance.'
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
