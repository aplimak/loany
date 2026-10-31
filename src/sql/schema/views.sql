CREATE VIEW IF NOT EXISTS loan_balances AS
SELECT
    id,
    person_id,
    direction,
    principal_amount,
    description,
    date_issued,
    due_date,
    paid_amount,
    principal_amount - paid_amount AS outstanding,
    CASE
        WHEN paid_amount >= principal_amount THEN 'settled'
        ELSE 'active'
    END AS status
FROM (
    SELECT
        l.id,
        l.person_id,
        l.direction,
        l.principal_amount,
        l.description,
        l.date_issued,
        l.due_date,
        COALESCE((
            SELECT SUM(p.amount) FROM payments AS p
            WHERE p.loan_id = l.id
        ), 0) AS paid_amount
    FROM loans AS l
) AS lb;
