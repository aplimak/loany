CREATE TRIGGER IF NOT EXISTS trg_loans_immutable
BEFORE UPDATE ON loans
BEGIN
    SELECT RAISE(ABORT, 'loans are immutable: delete and re-insert');
END;

CREATE TRIGGER IF NOT EXISTS trg_payments_immutable
BEFORE UPDATE ON payments
BEGIN
    SELECT RAISE(ABORT, 'payments are immutable: delete and re-insert');
END;
