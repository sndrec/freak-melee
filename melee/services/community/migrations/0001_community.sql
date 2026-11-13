CREATE TABLE news (
    id TEXT PRIMARY KEY,
    title TEXT NOT NULL,
    ticker TEXT NOT NULL,
    body TEXT NOT NULL,
    published_at INTEGER,
    updated_at INTEGER NOT NULL,
    revision INTEGER NOT NULL DEFAULT 1,
    archived INTEGER NOT NULL DEFAULT 0
);
CREATE INDEX news_history ON news(archived, published_at DESC, id DESC);

CREATE TABLE announcements (
    id TEXT PRIMARY KEY,
    delivery_id TEXT NOT NULL,
    title TEXT NOT NULL,
    body TEXT NOT NULL,
    news_id TEXT REFERENCES news(id),
    published_at INTEGER,
    expires_at INTEGER,
    withdrawn INTEGER NOT NULL DEFAULT 0,
    updated_at INTEGER NOT NULL,
    revision INTEGER NOT NULL DEFAULT 1
);
CREATE INDEX active_announcements ON announcements(withdrawn, published_at DESC);

CREATE TABLE feedback (
    id TEXT PRIMARY KEY,
    capture_at TEXT NOT NULL,
    build TEXT NOT NULL,
    created_at INTEGER NOT NULL,
    screenshot_key TEXT NOT NULL,
    context_key TEXT NOT NULL,
    notes_json TEXT NOT NULL,
    screenshot_sha256 TEXT NOT NULL,
    context_sha256 TEXT NOT NULL,
    screenshot_size INTEGER NOT NULL,
    context_size INTEGER NOT NULL,
    width INTEGER NOT NULL,
    height INTEGER NOT NULL,
    status TEXT NOT NULL DEFAULT 'new'
);
CREATE INDEX feedback_history ON feedback(created_at DESC, id DESC);
CREATE INDEX feedback_screenshot ON feedback(screenshot_key);
CREATE INDEX feedback_context ON feedback(context_key);

CREATE TABLE service_state (key TEXT PRIMARY KEY, value TEXT NOT NULL);
