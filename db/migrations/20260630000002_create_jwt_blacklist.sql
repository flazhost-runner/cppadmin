-- migrate:up
CREATE TABLE IF NOT EXISTS jwt_blacklist (
    jti        VARCHAR(36)  NOT NULL PRIMARY KEY,
    expires_at TIMESTAMP    NOT NULL
);
CREATE INDEX idx_jwt_blacklist_exp ON jwt_blacklist(expires_at);

-- migrate:down
-- Dropping the table drops its index; a standalone `DROP INDEX IF EXISTS` is not
-- portable (MySQL has no IF EXISTS on DROP INDEX, and requires an ON <table>).
DROP TABLE IF EXISTS jwt_blacklist;
