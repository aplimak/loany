CREATE TABLE IF NOT EXISTS loans (
    id INTEGER PRIMARY KEY,
    person_id INTEGER NOT NULL REFERENCES people (
        id
    ) ON DELETE RESTRICT CHECK (typeof(person_id) = 'integer'),
    direction TEXT NOT NULL CHECK (direction IN ('lent', 'borrowed')),
    principal_amount INTEGER NOT NULL CHECK (
        typeof(principal_amount) = 'integer' AND principal_amount > 0
    ),
    description TEXT,
    date_issued INTEGER NOT NULL DEFAULT (
        cast(strftime('%s', 'now') AS INTEGER)
    ) CHECK (typeof(date_issued) = 'integer' AND date_issued > 0),
    due_date INTEGER CHECK (
        due_date IS NULL OR (typeof(due_date) = 'integer' AND due_date > 0)
    )
);
