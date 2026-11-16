import { DurableObject } from "cloudflare:workers";

interface Membership extends Record<string, SqlStorageValue> {
  room: string;
  connection: string;
}

// One object per verified account or private guest installation key. A new
// membership fences the old one before requesting its removal from the room.
export class PlayerSession extends DurableObject<CloudflareBindings> {
  constructor(ctx: DurableObjectState, env: CloudflareBindings) {
    super(ctx, env);
    ctx.storage.sql.exec(`CREATE TABLE IF NOT EXISTS membership (
      id INTEGER PRIMARY KEY CHECK(id=1), room TEXT NOT NULL, connection TEXT NOT NULL)`);
  }

  async enter(room: string, connection: string): Promise<boolean> {
    const previous = this.ctx.storage.sql.exec<Membership>("SELECT room, connection FROM membership WHERE id=1").toArray()[0];
    this.ctx.storage.sql.exec("INSERT OR REPLACE INTO membership VALUES (1,?,?)", room, connection);
    if (previous && previous.connection !== connection) {
      try {
        await this.env.ROOMS.get(this.env.ROOMS.idFromString(previous.room)).evictConnection(previous.connection, previous.room === room);
      } catch (error) {
        if (this.owns(connection))
          this.ctx.storage.sql.exec("INSERT OR REPLACE INTO membership VALUES (1,?,?)", previous.room, previous.connection);
        throw error;
      }
    }
    return this.owns(connection);
  }

  owns(connection: string): boolean {
    return this.ctx.storage.sql.exec<Membership>("SELECT connection FROM membership WHERE id=1").toArray()[0]?.connection === connection;
  }

  leave(connection: string): void {
    this.ctx.storage.sql.exec("DELETE FROM membership WHERE id=1 AND connection=?", connection);
  }
}
