CREATE TABLE feedback_uploads (
    object_prefix TEXT PRIMARY KEY,
    bytes INTEGER NOT NULL,
    expires_at INTEGER NOT NULL
);
CREATE INDEX feedback_attachment_sizes ON feedback(screenshot_size, context_size);
