INSERT INTO people (name, phone, email, created_at)
VALUES (?1, ?2, ?3, COALESCE(?4, CAST(STRFTIME('%s', 'now') AS INTEGER)))
RETURNING id, created_at;
