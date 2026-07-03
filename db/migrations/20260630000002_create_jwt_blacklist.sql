-- migrate:up
CREATE TABLE IF NOT EXISTS jwt_blacklist (
    jti        VARCHAR(36)  NOT NULL PRIMARY KEY,
    expires_at TIMESTAMP    NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_jwt_blacklist_exp ON jwt_blacklist(expires_at);

-- migrate:down
DROP INDEX IF EXISTS idx_jwt_blacklist_exp;
DROP TABLE IF EXISTS jwt_blacklist;
