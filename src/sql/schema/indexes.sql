CREATE INDEX IF NOT EXISTS idx_loans_person_id ON loans (person_id);
CREATE INDEX IF NOT EXISTS idx_payments_loan_id ON payments (loan_id);
