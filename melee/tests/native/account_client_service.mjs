// Run with node --experimental-strip-types after building native_account_test.
// Uses real WebAuthn proofs on a local worker; never opens Windows Hello.
import assert from 'node:assert/strict';
import { spawn } from 'node:child_process';
import { mkdir, open, writeFile, readFile } from 'node:fs/promises';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { createInterface } from 'node:readline';
import { authenticator } from '../../services/signaling/test/account-authenticator.ts';

const root = resolve(dirname(fileURLToPath(import.meta.url)), '../../..');
const output = resolve(process.argv[2]);
const port = Number(process.argv[3] || 8797);
const endpoint = `http://localhost:${port}`;
const exe = resolve(root, 'melee/build/native/native_account_test.exe');
const serviceRoot = resolve(root, 'melee/services/signaling');
await mkdir(output, { recursive: false });
const log = await open(resolve(output, 'service.log'), 'w');
const service = spawn(process.execPath, [resolve(serviceRoot, 'node_modules/wrangler/bin/wrangler.js'),
  'dev', '--local', '--ip', '127.0.0.1', '--port', String(port), '--persist-to', resolve(output, 'service-state'),
  '--var', `AUTH_ORIGIN:${endpoint}`, '--var', 'AUTH_RP_ID:localhost'],
  { cwd: serviceRoot, windowsHide: true, stdio: ['ignore', log.fd, log.fd] });
await writeFile(resolve(output, 'service-pid.txt'), String(service.pid));
const children = [];
const checks = [];
let client;
async function request(handle, path, body, token) {
  const response = await fetch(`${endpoint}/v1/accounts/${handle}/${path}`, {
    method: body === undefined ? 'GET' : 'POST',
    headers: { 'Content-Type': 'application/json', ...(token ? { Authorization: `Bearer ${token}` } : {}) },
    body: body === undefined ? undefined : JSON.stringify(body),
  });
  const json = await response.json();
  assert.equal(response.status, 200, `service request ${path} failed: ${json.error}`);
  return json;
}
function native(resume = false) {
  const process = spawn(exe, [resume ? '--resume' : '--client', resolve(output, 'profile'), endpoint],
    { windowsHide: true, stdio: ['pipe', 'pipe', 'pipe'] });
  children.push(process);
  const lines = createInterface({ input: process.stdout })[Symbol.asyncIterator]();
  let error = '';
  process.stderr.on('data', data => { error += data; });
  return {
    process,
    async line() {
      let timer;
      try {
        const result = await Promise.race([lines.next(), new Promise((_, reject) => {
          timer = setTimeout(() => reject(new Error(`native account request timed out: ${error}`)), 35000);
        })]);
        assert(!result.done, `native account client exited: ${error}`);
        return result.value;
      } finally { clearTimeout(timer); }
    },
    async send(body) { process.stdin.write(JSON.stringify(body) + '\n'); return this.line(); },
    async close() {
      process.stdin.end();
      const code = await new Promise(resolve => process.once('exit', resolve));
      assert.equal(code, 0, `native client failed: ${error}`);
    },
  };
}
try {
  for (let i = 0; ; ++i) {
    try { if ((await fetch(`http://127.0.0.1:${port}/health`)).ok) break; } catch {}
    assert(i < 100, 'local account service did not start');
    await new Promise(resolve => setTimeout(resolve, 200));
  }
  const handle = 'native_test';
  const key = await authenticator();
  const options = await request(handle, 'register/options', { name: 'Native test' });
  const grant = await request(handle, 'register/finish', { attempt: options.attempt,
    credential: await key.register(options.options, options.origin) });
  assert(grant.recovery_code);
  const second = await authenticator();
  const add = await request(handle, 'add/options', {}, grant.token);
  await request(handle, 'add/finish', { attempt: add.attempt,
    credential: await second.register(add.options, add.origin) }, grant.token);
  const login = await request(handle, 'login/options', {});
  const other = await request(handle, 'login/finish', { attempt: login.attempt,
    credential: await key.sign(login.options, login.origin) });
  assert(other.token !== grant.token);
  await mkdir(resolve(output, 'profile'));
  client = native();
  client.process.stdin.write(JSON.stringify({ handle, token: grant.token,
    registration: options, authentication: login }) + '\n');
  assert.match(await client.line(), /ready signed-in=1 pending=0 keys=2 sessions=2/);
  checks.push('Native WinHTTP accepts a service-issued DPAPI-protected session and reads real profile/key/device lists');
  let line = await client.send({ operation: 6, name: 'Twilight test', bio: 'Native account profile' });
  assert.match(line, /name=Twilight test bio=Native account profile status=Account updated/);
  const updated = await request(handle, 'profile');
  assert.equal(updated.profile.name, 'Twilight test');
  checks.push('Native profile editing updates persistent service state');
  line = await client.send({ operation: 6, name: '', bio: 'Rejected edit' });
  assert.match(line, /name=Twilight test bio=Native account profile status=Enter a display name/);
  checks.push('A rejected profile edit preserves the last confirmed account profile');
  const me = await request(handle, 'me', undefined, grant.token);
  const otherSession = me.sessions.find(session => !session.current);
  line = await client.send({ operation: 8, target: otherSession.id });
  assert.match(line, /signed-in=1.*sessions=1/);
  checks.push('Native device revocation removes another real session');
  line = await client.send({ operation: 10, target: second.id });
  assert.match(line, /signed-in=1.*keys=1/);
  checks.push('Native passkey management removes a replacement while retaining the last key');
  line = await client.send({ operation: 10, target: key.id });
  assert.match(line, /Keep at least one passkey/);
  checks.push('Last-passkey removal is rejected without losing the active account');
  line = await client.send({ operation: 11 });
  assert.match(line, /saved=1 pending=1/);
  await client.close();
  const encrypted = await readFile(resolve(output, 'profile/account-session.bin'));
  assert(!encrypted.includes(Buffer.from(grant.token)) && !encrypted.includes(Buffer.from(grant.recovery_code)));
  client = native(true);
  assert.match(await client.line(), /ready signed-in=1 pending=1 keys=1 sessions=1/);
  checks.push('New recovery code stays protected and pending across a client restart');
  line = await client.send({ operation: 7 });
  assert.match(line, /Save your recovery code before signing out/);
  line = await client.send({ operation: 12 });
  assert.match(line, /pending=0/);
  line = await client.send({ operation: 7 });
  assert.match(line, /signed-in=0 saved=0 pending=0/);
  await client.close();
  checks.push('Recovery acknowledgement and sign-out remove the protected local session');
  await writeFile(resolve(output, 'receipt.json'), JSON.stringify({ checks, limitations: [
    'Software-authenticator proofs validate the real service, not the Windows Hello ceremony.',
    'No production deployment or authenticated social-presence claim.'
  ] }, null, 2));
  console.log(`${checks.length} native account/service checks passed`);
} finally {
  for (const child of children) if (child.exitCode === null) child.kill();
  if (service.exitCode === null) {
    const killer = spawn('taskkill.exe', ['/PID', String(service.pid), '/T', '/F'], { windowsHide: true, stdio: 'ignore' });
    await new Promise(resolve => killer.once('exit', resolve));
  }
  await log.close();
}
