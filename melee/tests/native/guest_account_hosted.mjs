// Uses task-owned accounts and an unlisted lobby; sends no chat messages.
import assert from 'node:assert/strict';
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';
const require = createRequire(new URL('../../services/signaling/package.json', import.meta.url));
const { build } = require('esbuild');

export async function reviewGuestAccount(endpoint, a, b, call, checks) {
  // Bundle the service's wire codec, so this check uses its actual protocol.
  const compiled = await build({ entryPoints: [fileURLToPath(new URL('../../services/signaling/src/wire.ts', import.meta.url))],
    bundle: true, write: false, format: 'esm', platform: 'neutral' });
  const wire = await import(`data:text/javascript;base64,${Buffer.from(compiled.outputFiles[0].contents).toString('base64')}`);
  const room = crypto.randomUUID().replaceAll('-', '').slice(0, 12).toUpperCase();
  const clients = [];
  async function connect() {
    const peer = `test_${crypto.randomUUID().replaceAll('-', '')}`;
    const url = new URL(`/v1/rooms/${room}/connect`, endpoint);
    url.protocol = url.protocol === 'https:' ? 'wss:' : 'ws:';
    url.search = new URLSearchParams({ peer, name: 'TEST guest', build: 'hosted-account-attach', players: '1' });
    const socket = new WebSocket(url); socket.binaryType = 'arraybuffer';
    const messages = []; let wake, failed;
    socket.addEventListener('message', event => {
      try { messages.push(wire.decodeServerMessage(event.data)); }
      catch { failed = new Error('Hosted room sent an invalid packet'); }
      wake?.();
    });
    socket.addEventListener('error', () => { failed = new Error('Hosted room connection failed'); wake?.(); });
    socket.addEventListener('close', () => { failed = new Error('Hosted room connection closed'); wake?.(); });
    async function next(type, predicate = () => true) {
      const deadline = Date.now() + 15000;
      for (;;) {
        if (failed) throw failed;
        const index = messages.findIndex(message => message.type === type && predicate(message));
        if (index >= 0) return messages.splice(index, 1)[0];
        if (Date.now() >= deadline) throw new Error(`Hosted room packet ${type} timed out`);
        await new Promise(resolve => {
          const timer = setTimeout(() => { wake = undefined; resolve(); }, Math.min(1000, deadline - Date.now()));
          wake = () => { clearTimeout(timer); wake = undefined; resolve(); };
        });
      }
    }
    const client = { socket, next, peer }; clients.push(client);
    client.welcome = await next(wire.ServerMessageType.Welcome);
    return client;
  }
  try {
    const owner = await connect(), observer = await connect();
    const before = (await owner.next(wire.ServerMessageType.Presence, p => p.snapshot.participants.length === 2)).snapshot;
    const body = { room, connection: owner.welcome.connectionId };
    await call(a, 'lobby/attach', { ...body, connection: owner.peer }, a.grant.token, [400]);
    await call(a, 'lobby/attach', { ...body, connection: crypto.randomUUID() }, a.grant.token, [409]);
    await call(a, 'lobby/attach', body);
    const attached = p => p.snapshot.participants.some(member => member.peerId === owner.peer && member.accountId === a.grant.profile.id);
    const after = (await owner.next(wire.ServerMessageType.Presence, attached)).snapshot;
    assert.deepEqual({ ...after, revision: before.revision, participants: before.participants }, before);
    assert.deepEqual(after.participants, before.participants.map(member => member.peerId !== owner.peer ? member : {
      ...member, accountId: a.grant.profile.id, accountHandle: a.handle, name: a.grant.profile.name,
    }));
    await observer.next(wire.ServerMessageType.Presence, attached);
    await call(b, 'lobby/attach', body, b.grant.token, [409]);
    await call(a, 'lobby/attach', body);
    assert.equal(owner.socket.readyState, WebSocket.OPEN);
    checks.push('Hosted guest-to-account attachment preserves the live socket, owner, seats and match metadata and reaches the second client');
    checks.push('Hosted attachment rejects public peer IDs, unknown connections and account replacement; repeated attachment is idempotent');
  } finally {
    for (const client of clients) {
      if (client.socket.readyState === WebSocket.CLOSED) continue;
      const closed = new Promise(resolve => {
        const timer = setTimeout(resolve, 5000);
        client.socket.addEventListener('close', () => { clearTimeout(timer); resolve(); }, { once: true });
      });
      client.socket.close(); await closed;
    }
  }
}
