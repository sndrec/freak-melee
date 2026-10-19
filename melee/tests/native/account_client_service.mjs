// Real native account-key generation, signatures, import/export and recovery.
import assert from 'node:assert/strict';
import { spawn } from 'node:child_process';
import { mkdir, open, writeFile, readFile, rm } from 'node:fs/promises';
import { createPrivateKey, createPublicKey, sign } from 'node:crypto';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { createInterface } from 'node:readline';

const root = resolve(dirname(fileURLToPath(import.meta.url)), '../../..');
const output = resolve(process.argv[2]), port = Number(process.argv[3] || 8797);
const endpoint = `http://localhost:${port}`;
const exe = resolve(root, 'melee/build/native/native_account_test.exe');
const serviceRoot = resolve(root, 'melee/services/signaling');
await mkdir(output, { recursive: false });
const log = await open(resolve(output, 'service.log'), 'w');
const service = spawn(process.execPath, [resolve(serviceRoot, 'node_modules/wrangler/bin/wrangler.js'),
  'dev', '--local', '--ip', '127.0.0.1', '--port', String(port), '--persist-to', resolve(output, 'service-state'),
  '--var', `AUTH_ORIGIN:${endpoint}`], { cwd: serviceRoot, windowsHide: true, stdio: ['ignore', log.fd, log.fd] });
const children = [], checks = [], secrets = [];
const handle = 'native_key_test';
async function call(route, body, token) {
  const response = await fetch(`${endpoint}/v1/accounts/${handle}/${route}`, {
    method: body === undefined ? 'GET' : 'POST',
    headers: { 'Content-Type': 'application/json', ...(token ? { Authorization: `Bearer ${token}` } : {}) },
    body: body === undefined ? undefined : JSON.stringify(body),
  });
  const result = await response.json(); assert.equal(response.status, 200, `${route}: ${result.error}`); return result;
}
function identity(file) {
  const key = createPrivateKey({ key: Buffer.concat([Buffer.from('302e020100300506032b657004220420', 'hex'), Buffer.from(file.seed, 'base64url')]), format: 'der', type: 'pkcs8' });
  const public_key = createPublicKey(key).export({ format: 'der', type: 'spki' }).subarray(-32).toString('base64url');
  return { public_key, async login() {
    const c = await call('login/options', { public_key });
    const message = ['Freak Melee account key', c.origin, handle, c.user_id, c.purpose, c.public_key, c.attempt, c.challenge].join('\n');
    return call('login/finish', { attempt: c.attempt, signature: sign(null, Buffer.from(message), key).toString('base64url') });
  } };
}
async function native(folder) {
  const profile = resolve(output, folder); await mkdir(profile, { recursive: true });
  const child = spawn(exe, ['--service', profile, endpoint], { windowsHide: true, stdio: ['pipe', 'pipe', 'pipe'] }); children.push(child);
  const lines = createInterface({ input: child.stdout })[Symbol.asyncIterator](); let error = '';
  child.stderr.on('data', data => { error += data; });
  async function line() {
    let timer;
    try {
      const result = await Promise.race([lines.next(), new Promise((_, reject) => {
        timer = setTimeout(() => reject(new Error(`Native request timed out: ${error}`)), 35000);
      })]);
      assert(!result.done, `Native client exited: ${error}`); return result.value;
    } finally { clearTimeout(timer); }
  }
  return { ready: await line(), profile,
    async send(body) { child.stdin.write(JSON.stringify(body) + '\n'); return line(); },
    async close() { const exited = new Promise(resolve => child.once('exit', resolve)); child.stdin.end(); assert.equal(await exited, 0, error); },
  };
}
try {
  for (let i = 0; ; ++i) {
    try { if ((await fetch(`${endpoint}/health`)).ok) break; } catch {}
    assert(i < 100, 'Local account service did not start'); await new Promise(resolve => setTimeout(resolve, 200));
  }
  let a = await native('a'); assert.match(a.ready, /signed-in=0/);
  assert.match(await a.send({ operation: 'Register', handle, name: 'Native key test' }), /signed-in=1 saved=1 pending=1 keys=1 sessions=1/);
  const originalID = (await call('profile')).profile.id;
  checks.push('Native-generated Ed25519 registration is accepted by the independent service verifier');
  const recoveryPath = resolve(output, 'recovery.txt'), keyPath = resolve(output, 'account.fmakey'), secondPath = resolve(output, 'additional.fmakey');
  secrets.push(recoveryPath, keyPath, secondPath);
  assert.match(await a.send({ operation: 'Logout' }), /Save your recovery code before signing out/);
  assert.match(await a.send({ operation: 'ExportRecovery', path: recoveryPath }), /pending=0/);
  assert.match(await a.send({ operation: 'ExportKey', path: keyPath }), /Account-key file saved/);
  const exported = JSON.parse(await readFile(keyPath, 'utf8'));
  assert.deepEqual(Object.keys(exported).sort(), ['account', 'application', 'seed', 'service']);
  const nodeKey = identity(exported), grant = await nodeKey.login();
  assert.equal(grant.profile.id, originalID);
  const encrypted = await readFile(resolve(a.profile, 'account-session.bin'));
  assert(!encrypted.includes(Buffer.from(exported.seed)) && !encrypted.includes(Buffer.from(grant.token)));
  checks.push('Portable native export interoperates with Node Ed25519; the local record protects secrets');
  await a.close(); a = await native('a'); assert.match(a.ready, /signed-in=1 pending=0/);
  checks.push('Saved sessions resume automatically without an OS prompt');
  assert.match(await a.send({ operation: 'SaveProfile', name: 'Twilight test', bio: 'Account-key profile' }), /name=Twilight test bio=Account-key profile status=Account updated/);
  assert.match(await a.send({ operation: 'SaveProfile', name: '', bio: 'Rejected' }), /name=Twilight test bio=Account-key profile status=Enter a display name/);
  assert.match(await a.send({ operation: 'AddKey', path: secondPath }), /signed-in=1.*keys=2.*New account-key file saved and registered/);
  const second = JSON.parse(await readFile(secondPath, 'utf8')); assert.notEqual(second.seed, exported.seed);
  let b = await native('b'); assert.match(await b.send({ operation: 'ImportKey', path: secondPath }), /signed-in=1 saved=1 pending=0 keys=2/);
  const secondKey = identity(second);
  assert.match(await a.send({ operation: 'RemoveKey', target: secondKey.public_key }), /signed-in=1.*keys=1/);
  assert.match(await b.send({ operation: 'Refresh' }), /signed-in=0/);
  await b.close(); b = await native('b'); assert.match(b.ready, /signed-in=0/); await b.close();
  checks.push('Additional device keys import normally; removal revokes their sessions and survives restart');
  assert.match(await a.send({ operation: 'RemoveKey', target: nodeKey.public_key }), /Keep at least one account key/);
  assert.match(await a.send({ operation: 'Logout' }), /signed-in=0 saved=1 pending=0/);
  await a.close(); a = await native('a'); assert.match(a.ready, /signed-in=0/);
  assert.match(await a.send({ operation: 'Login' }), /signed-in=1/);
  checks.push('Intentional sign-out stays signed out across restart and retains the local key for explicit sign-in');
  const recoveryText = await readFile(recoveryPath, 'utf8'), recovery = recoveryText.match(/\r\n([A-Za-z0-9_-]{43})\r\n/)[1];
  b = await native('recovered');
  assert.match(await b.send({ operation: 'Recover', handle, recovery }), /signed-in=1 saved=1 pending=1 keys=1 sessions=1/);
  assert.equal((await call('profile')).profile.id, originalID);
  assert.match(await a.send({ operation: 'Refresh' }), /signed-in=0/);
  assert.match(await a.send({ operation: 'Login' }), /signed-in=0/);
  assert.match(await b.send({ operation: 'ExportRecovery', path: recoveryPath }), /pending=0/);
  assert.match(await b.send({ operation: 'RotateRecovery' }), /saved=1 pending=1/);
  await b.close(); b = await native('recovered'); assert.match(b.ready, /signed-in=1 pending=1/);
  assert.match(await b.send({ operation: 'ExportRecovery', path: recoveryPath }), /pending=0/);
  assert.match(await b.send({ operation: 'RevokeAll' }), /signed-in=0/);
  await a.close(); await b.close();
  checks.push('Recovery preserves account identity, replaces keys/sessions, and protects new recovery codes across restart');
  await writeFile(resolve(output, 'receipt.json'), JSON.stringify({ checks, limitations: ['Native WinHTTP and files tested with a local service; file picker interaction and hosted deployment are separate checks.'] }, null, 2));
  console.log(`${checks.length} native account-key integration checks passed`);
} finally {
  for (const child of children) if (child.exitCode === null) child.kill();
  for (const path of secrets) await rm(path, { force: true });
  if (service.exitCode === null) {
    const killer = spawn('taskkill.exe', ['/PID', String(service.pid), '/T', '/F'], { windowsHide: true, stdio: 'ignore' });
    await new Promise(resolve => killer.once('exit', resolve));
  }
  await log.close();
}
