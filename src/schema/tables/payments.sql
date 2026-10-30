CREATE TABLE IF NOT EXISTS payments (
    id INTEGER PRIMARY KEY,
    loan_id INTEGER NOT NULL REFERENCES loans (id) ON DELETE RESTRICT CHECK (
        typeof(loan_id) = 'integer'
    ),
    amount INTEGER NOT NULL CHECK (typeof(amount) = 'integer' AND amount > 0),
    payment_date INTEGER NOT NULL DEFAULT (
        cast(strftime('%s', 'now') AS INTEGER)
    ) CHECK (typeof(payment_date) = 'integer' AND payment_date > 0),
    note TEXT
);
