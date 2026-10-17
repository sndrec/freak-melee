import { DurableObject } from "cloudflare:workers";
import { allocateQuickMatch, chooseQuickTickets, quickBucket, quickSeats,
  QUICK_SEARCH_LEASE_MS, validQuickOffer, type QuickOffer, type QuickTicket,
  type QuickAllocation } from "./quick-play";

interface TicketRow extends Record<string, SqlStorageValue> {
  party: string; generation: number; created: number; expires: number; offer: string;
}
interface FenceRow extends Record<string, SqlStorageValue> { generation: number; cancelled: number }
interface JobRow extends Record<string, SqlStorageValue> { id: string; allocation: string; cancelled: number; committed: number }

export class QuickPlayQueue extends DurableObject<CloudflareBindings> {
  private draining = false;
  constructor(ctx: DurableObjectState, env: CloudflareBindings) {
    super(ctx, env);
    ctx.blockConcurrencyWhile(async () => {
      ctx.storage.sql.exec(`
        CREATE TABLE IF NOT EXISTS queue_meta (id INTEGER PRIMARY KEY CHECK(id=1), bucket TEXT NOT NULL);
        CREATE TABLE IF NOT EXISTS queue_fences (party TEXT PRIMARY KEY, generation INTEGER NOT NULL, cancelled INTEGER NOT NULL);
        CREATE TABLE IF NOT EXISTS queue_entries (party TEXT PRIMARY KEY, generation INTEGER NOT NULL,
          created INTEGER NOT NULL, expires INTEGER NOT NULL, offer TEXT NOT NULL);
        CREATE TABLE IF NOT EXISTS queue_peers (peer TEXT PRIMARY KEY, party TEXT NOT NULL, generation INTEGER NOT NULL);
        CREATE TABLE IF NOT EXISTS queue_jobs (id TEXT PRIMARY KEY, allocation TEXT NOT NULL, cancelled INTEGER NOT NULL, committed INTEGER NOT NULL);
        CREATE TABLE IF NOT EXISTS queue_job_members (party TEXT PRIMARY KEY, generation INTEGER NOT NULL, job TEXT NOT NULL);
      `);
    });
  }

  async enqueue(offer: QuickOffer): Promise<boolean> {
    if (!validQuickOffer(offer)) throw new Error("Invalid Quick Play party");
    const bucket = quickBucket(offer.mode, offer.build);
    this.ctx.storage.sql.exec("INSERT OR IGNORE INTO queue_meta VALUES (1, ?)", bucket);
    if (this.ctx.storage.sql.exec<{bucket: string}>("SELECT bucket FROM queue_meta WHERE id=1").one().bucket !== bucket)
      throw new Error("Incorrect matchmaking bucket");
    const fence = this.ctx.storage.sql.exec<FenceRow>("SELECT generation, cancelled FROM queue_fences WHERE party=?", offer.partyId).toArray()[0];
    if (fence && (fence.generation > offer.generation || (fence.generation === offer.generation && fence.cancelled))) return false;
    for (const member of offer.members) {
      const existingPeer = this.ctx.storage.sql.exec<{party: string}>("SELECT party FROM queue_peers WHERE peer=?", member.peerId).toArray()[0];
      if (existingPeer && existingPeer.party !== offer.partyId) return false;
    }
    const pending = this.ctx.storage.sql.exec<{generation: number}>("SELECT generation FROM queue_job_members WHERE party=?", offer.partyId).toArray()[0];
    if (pending?.generation === offer.generation) return true;
    const now = Date.now();
    const existing = this.ctx.storage.sql.exec<TicketRow>("SELECT * FROM queue_entries WHERE party=?", offer.partyId).toArray()[0];
    if (!existing && this.ctx.storage.sql.exec<{count: number}>("SELECT COUNT(*) AS count FROM queue_entries").one().count >= 512)
      return false;
    this.ctx.storage.transactionSync(() => {
      this.ctx.storage.sql.exec("UPDATE queue_jobs SET cancelled=1 WHERE id IN (SELECT job FROM queue_job_members WHERE party=? AND generation<?)", offer.partyId, offer.generation);
      this.ctx.storage.sql.exec("INSERT OR REPLACE INTO queue_fences VALUES (?, ?, 0)", offer.partyId, offer.generation);
      this.ctx.storage.sql.exec("INSERT OR REPLACE INTO queue_entries VALUES (?, ?, ?, ?, ?)", offer.partyId, offer.generation,
        existing?.generation === offer.generation ? existing.created : now, now + QUICK_SEARCH_LEASE_MS, JSON.stringify(offer));
      for (const member of offer.members)
        this.ctx.storage.sql.exec("INSERT OR REPLACE INTO queue_peers VALUES (?, ?, ?)", member.peerId, offer.partyId, offer.generation);
    });
    await this.schedule();
    this.ctx.waitUntil(this.drain());
    return true;
  }

  async cancel(party: string, generation: number): Promise<void> {
    this.ctx.storage.transactionSync(() => {
      const fence = this.ctx.storage.sql.exec<FenceRow>("SELECT generation, cancelled FROM queue_fences WHERE party=?", party).toArray()[0];
      if (!fence || generation >= fence.generation)
        this.ctx.storage.sql.exec("INSERT OR REPLACE INTO queue_fences VALUES (?, ?, 1)", party, generation);
      this.ctx.storage.sql.exec("DELETE FROM queue_entries WHERE party=? AND generation<=?", party, generation);
      this.ctx.storage.sql.exec("DELETE FROM queue_peers WHERE party=? AND generation<=?", party, generation);
      this.ctx.storage.sql.exec("UPDATE queue_jobs SET cancelled=1 WHERE id IN (SELECT job FROM queue_job_members WHERE party=? AND generation<=?)", party, generation);
    });
    await this.schedule();
    this.ctx.waitUntil(this.drain());
  }

  async alarm(): Promise<void> { await this.drain(); }

  private async schedule(): Promise<void> {
    const pending = this.ctx.storage.sql.exec<{count: number}>("SELECT COUNT(*) AS count FROM queue_jobs").one().count;
    const deadline = this.ctx.storage.sql.exec<{deadline: number | null}>("SELECT MIN(expires) AS deadline FROM queue_entries").one().deadline;
    const next = pending ? Date.now() + 1000 : deadline;
    if (next === null) await this.ctx.storage.deleteAlarm();
    else await this.ctx.storage.setAlarm(Math.max(Date.now() + 1, next));
  }

  private jobAlive(id: string): boolean {
    const row = this.ctx.storage.sql.exec<JobRow>("SELECT * FROM queue_jobs WHERE id=?", id).toArray()[0];
    return row?.cancelled === 0 && (row.committed !== 0 ||
      (JSON.parse(row.allocation) as QuickAllocation).parties.every((party) => party.expires > Date.now()));
  }
  private async expire(): Promise<void> {
    const expired = this.ctx.storage.sql.exec<TicketRow>("SELECT * FROM queue_entries WHERE expires<=?", Date.now()).toArray();
    for (const row of expired) {
      this.ctx.storage.sql.exec("DELETE FROM queue_entries WHERE party=? AND generation=?", row.party, row.generation);
      this.ctx.storage.sql.exec("DELETE FROM queue_peers WHERE party=? AND generation=?", row.party, row.generation);
      this.ctx.storage.sql.exec("UPDATE queue_fences SET cancelled=1 WHERE party=? AND generation=?", row.party, row.generation);
      await this.env.ROOMS.get(this.env.ROOMS.idFromString(row.party)).expireQuickSearch(row.generation);
    }
  }

  private reserve(): JobRow | null {
    const old = this.ctx.storage.sql.exec<JobRow>("SELECT * FROM queue_jobs ORDER BY rowid LIMIT 1").toArray()[0];
    if (old) return old;
    const rows = this.ctx.storage.sql.exec<TicketRow>("SELECT * FROM queue_entries ORDER BY created, rowid").toArray();
    if (!rows.length) return null;
    const tickets = rows.map((row) => ({ ...JSON.parse(row.offer) as QuickOffer, created: row.created, expires: row.expires }));
    const chosen = chooseQuickTickets(tickets, tickets[0].mode);
    if (!chosen.length) return null;
    const allocation = allocateQuickMatch(chosen);
    const encoded = JSON.stringify(allocation);
    this.ctx.storage.transactionSync(() => {
      this.ctx.storage.sql.exec("INSERT INTO queue_jobs VALUES (?, ?, 0, 0)", allocation.id, encoded);
      for (const party of chosen) {
        this.ctx.storage.sql.exec("DELETE FROM queue_entries WHERE party=? AND generation=?", party.partyId, party.generation);
        this.ctx.storage.sql.exec("INSERT INTO queue_job_members VALUES (?, ?, ?)", party.partyId, party.generation, allocation.id);
      }
    });
    return { id: allocation.id, allocation: encoded, cancelled: 0, committed: 0 };
  }

  private async deliver(job: JobRow): Promise<void> {
    const allocation = JSON.parse(job.allocation) as QuickAllocation;
    if (!job.committed && allocation.parties.some((party) => party.expires <= Date.now()))
      this.ctx.storage.sql.exec("UPDATE queue_jobs SET cancelled=1 WHERE id=?", job.id);
    if (!job.committed && this.jobAlive(job.id)) {
      for (const party of allocation.parties) {
        const prepared = await this.env.ROOMS.get(this.env.ROOMS.idFromString(party.partyId)).prepareQuickAllocation(party.generation, allocation);
        if (!prepared || !this.jobAlive(job.id)) {
          this.ctx.storage.sql.exec("UPDATE queue_jobs SET cancelled=1 WHERE id=?", job.id);
          break;
        }
      }
    }
    if (!job.committed && this.jobAlive(job.id)) {
      if (!await this.env.ROOMS.getByName(allocation.roomCode).configureQuickMatch(allocation))
        this.ctx.storage.sql.exec("UPDATE queue_jobs SET cancelled=1 WHERE id=?", job.id);
      else if (this.jobAlive(job.id))
        this.ctx.storage.sql.exec("UPDATE queue_jobs SET committed=1 WHERE id=?", job.id);
    }
    if (this.jobAlive(job.id)) {
      for (const party of allocation.parties) {
        const activated = await this.env.ROOMS.get(this.env.ROOMS.idFromString(party.partyId)).activateQuickAllocation(party.generation, allocation.id);
        if (!activated || !this.jobAlive(job.id)) {
          this.ctx.storage.sql.exec("UPDATE queue_jobs SET cancelled=1 WHERE id=?", job.id);
          break;
        }
      }
    }
    if (!this.jobAlive(job.id)) {
      await this.env.ROOMS.getByName(allocation.roomCode).abortQuickAllocation(allocation.id);
      for (const party of allocation.parties) {
        const retry = await this.env.ROOMS.get(this.env.ROOMS.idFromString(party.partyId)).releaseQuickAllocation(party.generation, allocation.id);
        if (retry && party.expires <= Date.now()) {
          await this.env.ROOMS.get(this.env.ROOMS.idFromString(party.partyId)).expireQuickSearch(party.generation);
        } else if (retry) {
          const fence = this.ctx.storage.sql.exec<FenceRow>("SELECT generation, cancelled FROM queue_fences WHERE party=?", party.partyId).toArray()[0];
          if (fence?.generation === retry.generation && !fence.cancelled)
            this.ctx.storage.sql.exec("INSERT OR REPLACE INTO queue_entries VALUES (?, ?, ?, ?, ?)",
              party.partyId, retry.generation, party.created, party.expires, JSON.stringify(retry));
        }
      }
    } else {
      for (const party of allocation.parties)
        this.ctx.storage.sql.exec("UPDATE queue_fences SET cancelled=1 WHERE party=? AND generation=?", party.partyId, party.generation);
    }
    this.ctx.storage.transactionSync(() => {
      for (const party of allocation.parties) {
        const queued = this.ctx.storage.sql.exec<{generation: number}>("SELECT generation FROM queue_entries WHERE party=?", party.partyId).toArray()[0];
        if (queued?.generation !== party.generation)
          this.ctx.storage.sql.exec("DELETE FROM queue_peers WHERE party=? AND generation=?", party.partyId, party.generation);
      }
      this.ctx.storage.sql.exec("DELETE FROM queue_job_members WHERE job=?", job.id);
      this.ctx.storage.sql.exec("DELETE FROM queue_jobs WHERE id=?", job.id);
    });
  }

  private async drain(): Promise<void> {
    if (this.draining) return;
    this.draining = true;
    try {
      await this.expire();
      for (;;) {
        const job = this.reserve();
        if (!job) break;
        await this.deliver(job);
      }
    } catch (error) {
      console.error(JSON.stringify({ message: "quick play allocation deferred", error: String(error) }));
    } finally {
      this.draining = false;
      await this.schedule();
    }
  }
}
