import { exports } from "cloudflare:workers";
import { decodeServerMessage, ServerMessageType, type ServerMessage,
  encodeConfigureQuick, encodeQueueQuick, encodeRequestQuick } from "../src/wire";
import type { QuickState } from "../src/quick-play";
const worker = exports.default as unknown as Fetcher;
export class QuickClient {
  private messages: ServerMessage[] = [];
  private receiving = Promise.resolve();
  private changed?: () => void;
  private error?: unknown;
  constructor(readonly socket: WebSocket) {
    socket.addEventListener("message", (event) => {
      this.receiving = this.receiving.then(async () => {
        const data: unknown = event.data;
        let bytes: ArrayBuffer;
        if (ArrayBuffer.isView(data)) bytes = new Uint8Array(data.buffer, data.byteOffset, data.byteLength).slice().buffer;
        else if (typeof data === "object" && data !== null && "arrayBuffer" in data && typeof data.arrayBuffer === "function") bytes = await data.arrayBuffer() as ArrayBuffer;
        else bytes = data as ArrayBuffer;
        this.messages.push(decodeServerMessage(bytes));
        this.changed?.();
      }).catch((error: unknown) => { this.error = error; this.changed?.(); });
    });
    socket.accept();
  }
  async next<T extends ServerMessageType>(type: T): Promise<Extract<ServerMessage, {type: T}>> {
    for (;;) {
      if (this.error) throw this.error;
      const index = this.messages.findIndex((message) => message.type === type);
      if (index >= 0) return this.messages.splice(index, 1)[0] as Extract<ServerMessage, {type: T}>;
      await new Promise<void>((resolve, reject) => {
        const timer = setTimeout(() => { this.changed = undefined; reject(new Error(`Quick message ${type} timed out`)); }, 3000);
        this.changed = () => { clearTimeout(timer); this.changed = undefined; resolve(); };
      });
    }
  }
  async state(expected?: number): Promise<QuickState> {
    for (;;) {
      const message = await this.next(ServerMessageType.QuickState);
      if (expected === undefined || message.quick.state === expected) return message.quick;
    }
  }
  async configure(mode: number) { this.socket.send(encodeConfigureQuick(mode)); return this.state(0); }
  async search() { this.socket.send(encodeQueueQuick(true)); }
  async cancel() { this.socket.send(encodeQueueQuick(false)); return this.state(0); }
  async refresh() { this.socket.send(encodeRequestQuick()); return this.state(); }
  close() { this.socket.close(1000, "Test finished"); }
}
export async function quickConnect(room: string, peer: string, players = 1,
  build = "quick-test", admission?: string): Promise<Response> {
  const query = new URLSearchParams({ peer, name: peer, build, players: String(players) });
  if (admission) query.set("admission", admission);
  return worker.fetch(`https://test/v1/rooms/${room}/connect?${query}`, { headers: {Upgrade: "websocket"} });
}
export async function quickJoin(room: string, peer: string, players = 1, build = "quick-test", admission?: string): Promise<QuickClient> {
  const response = await quickConnect(room, peer, players, build, admission);
  if (response.status !== 101 || !response.webSocket) throw new Error(`Quick join failed ${response.status}: ${await response.text()}`);
  const client = new QuickClient(response.webSocket);
  await client.next(ServerMessageType.Welcome);
  await client.next(ServerMessageType.Presence);
  return client;
}
