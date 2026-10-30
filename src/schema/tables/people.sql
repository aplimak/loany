CREATE TABLE IF NOT EXISTS people (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL CHECK (length(trim(name)) > 0),
    phone TEXT,
    email TEXT,
    created_at INTEGER NOT NULL DEFAULT (
        cast(strftime('%s', 'now') AS INTEGER)
    ) CHECK (typeof(created_at) = 'integer' AND created_at > 0)
);
