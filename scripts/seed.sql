-- seed.sql
PRAGMA foreign_keys = ON;      -- fail loudly if FKs break
PRAGMA journal_mode = WAL;     -- better for bulk insert
PRAGMA synchronous  = OFF;     -- safe for throwaway data, big speedup

BEGIN;

-- ---------- people : 10,000 rows ----------
WITH RECURSIVE seq(n) AS (
    SELECT 1
    UNION ALL
    SELECT n + 1 FROM seq WHERE n < 10000
)
INSERT INTO people (id, name, phone, email)
SELECT
    n,
    'person ' || n,
    '+1-555-' || printf('%04d', n % 10000),
    'p' || n || '@example.com'
FROM seq;

-- ---------- loans : 50,000 rows ----------
WITH RECURSIVE seq(n) AS (
    SELECT 1
    UNION ALL
    SELECT n + 1 FROM seq WHERE n < 50000
)
INSERT INTO loans (id, person_id, direction, principal_amount,
                   description, date_issued, due_date)
SELECT
    n,
    abs(random() % 10000) + 1,
    CASE abs(random() % 2) WHEN 0 THEN 'lent' ELSE 'borrowed' END,
    abs(random() % 1000000) + 100,                 -- 100 .. 1,000,099
    'loan #' || n,
    abs(random() % 1000000000) + 1,
    CASE abs(random() % 4)
        WHEN 0 THEN NULL                            -- 25% no due date
        ELSE abs(random() % 1000000000) + 1
    END
FROM seq;

-- ---------- payments : 200,000 rows ----------
WITH RECURSIVE seq(n) AS (
    SELECT 1
    UNION ALL
    SELECT n + 1 FROM seq WHERE n < 200000
)
INSERT INTO payments (id, loan_id, amount, payment_date, note)
SELECT
    n,
    abs(random() % 50000) + 1,                     -- spread across all loans
    abs(random() % 50000) + 1,
    abs(random() % 1000000000) + 1,
    'payment #' || n
FROM seq;

COMMIT;

-- Refresh planner statistics so EXPLAIN QUERY PLAN is meaningful
ANALYZE;

-- Sanity checks
SELECT 'people'   AS t, count(*) FROM people
UNION ALL
SELECT 'loans',         count(*) FROM loans
UNION ALL
SELECT 'payments',      count(*) FROM payments;
PRAGMA foreign_key_check;
