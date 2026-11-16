// Run against an isolated local signaling service. Simulate an outage without
// touching the PC's network adapter or any user-owned game process.
import { createRequire } from 'node:module';
import { createServer } from 'node:http';
import { spawn } from 'node:child_process';
import { mkdir, writeFile } from 'node:fs/promises';
import { resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const require = createRequire(new URL('../services/signaling/package.json', import.meta.url));
const { WebSocket, WebSocketServer } = require('ws');
const args = Object.fromEntries(process.argv.slice(2).reduce((pairs, value, index, all) =>
  index % 2 ? pairs : [...pairs, [value.replace(/^--/, ''), all[index + 1]]], []));
if (!args.exe || !args.endpoint || !args.output) throw new Error('Required: --exe PATH --endpoint LOCAL_URL --output DIRECTORY');
const endpoint = new URL(args.endpoint);
if (!['localhost', '127.0.0.1', '[::1]'].includes(endpoint.hostname)) throw new Error('Use an isolated local service');
const output = resolve(args.output), connections = new Set();
const replaced = args.mode === 'replaced', chat = args.kind === 'chat';
await mkdir(output, { recursive: true });
let started = 0, timer, replacementTimer, dropped = 0, opened = 0, child;
const proxy = new WebSocketServer({ noServer: true });
const server = createServer((_request, response) => { response.writeHead(404); response.end(); });
server.on('upgrade', (request, socket, head) => {
  const age = Date.now() - started;
  if (!replaced && started && age >= 5000 && age < 60000) { socket.destroy(); return; }
  proxy.handleUpgrade(request, socket, head, client => {
    ++opened;
    if (!started) {
      started = Date.now();
      if (replaced) replacementTimer = setTimeout(() => {
        for (const pair of connections) pair.client.close(4001, 'Moved to another lobby');
      }, 5000);
    }
    const url = new URL(request.url, endpoint); url.protocol = endpoint.protocol === 'https:' ? 'wss:' : 'ws:';
    const upstream = new WebSocket(url, { headers: { 'X-Freak-Guest': request.headers['x-freak-guest'] } });
    const pair = { client, upstream }; connections.add(pair);
    const pending = [];
    const blackedOut = () => !replaced && Date.now() - started >= 5000 && Date.now() - started < 60000;
    client.on('message', (bytes, binary) => {
      if (blackedOut()) { ++dropped; return; }
      if (upstream.readyState === WebSocket.OPEN) upstream.send(bytes, { binary });
      else if (pending.length < 16) pending.push({ bytes, binary });
      else client.terminate();
    });
    upstream.on('open', () => { for (const packet of pending) upstream.send(packet.bytes, { binary: packet.binary }); pending.length = 0; });
    upstream.on('message', (bytes, binary) => {
      if (blackedOut()) { ++dropped; return; }
      if (client.readyState === WebSocket.OPEN) client.send(bytes, { binary });
    });
    const close = () => { connections.delete(pair); client.terminate(); upstream.terminate(); };
    client.on('close', close); upstream.on('close', close);
    client.on('error', close); upstream.on('error', close);
  });
});
try {
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  const port = server.address().port;
  console.log(replaced ? 'Checking that replaced clients stay disconnected.' : 'Holding native clients through a 55-second heartbeat outage.');
  const origin = `http://127.0.0.1:${port}`;
  const command = chat ? ['--presence', origin, 'Probe player', 'Probe player', replaced ? '15' : '75'] :
    [origin, '--hold', replaced ? '15' : '75'];
  child = spawn(resolve(args.exe), command, { windowsHide: true });
  let log = '';
  child.stdout.on('data', chunk => { log += chunk; }); child.stderr.on('data', chunk => { log += chunk; });
  timer = setTimeout(() => child.kill(), 100000);
  const code = await new Promise((resolve, reject) => { child.on('exit', resolve); child.on('error', reject); });
  const result = /hold welcomes=(\d+),(\d+) heartbeat-fresh=1/.exec(log);
  const passed = replaced ? opened === (chat ? 1 : 2) && code === (chat ? 0 : 1) && (!chat || /state=3 online=0/.test(log)) :
    chat ? code === 0 && opened >= 2 && (log.match(/state=1 online=/g)?.length ?? 0) >= 2 && dropped > 0 :
      code === 0 && result && +result[1] >= 2 && +result[2] >= 2 && dropped > 0;
  await writeFile(resolve(output, 'probe.log'), log);
  await writeFile(resolve(output, 'review.json'), JSON.stringify({ passed: !!passed, exit: code,
    welcomes: result?.slice(1).map(Number), connectionsOpened: opened, replaced, chat,
    droppedMessages: dropped, source: fileURLToPath(import.meta.url) }, null, 2));
  if (!passed) throw new Error(`Native recovery probe failed; see ${output}/probe.log`);
  console.log(replaced ? 'PASS: replaced clients did not reconnect.' :
    chat ? 'PASS: native chat reconnected; heartbeats resumed.' : 'PASS: both native clients reconnected; heartbeats resumed.');
} finally {
  clearTimeout(timer);
  clearTimeout(replacementTimer);
  if (child && child.exitCode === null) child.kill();
  for (const { client, upstream } of connections) { client.terminate(); upstream.terminate(); }
  proxy.close(); server.close();
}
