-- migrate:up
-- Seed the default admin user, the two default roles, and the setting singleton.
-- Runs wherever dbmate runs — INCLUDING production MySQL (the deploy entrypoint
-- runs `dbmate up`). Mirrors db/seeds/seed.sql (the sqlite-only dev seed) so
-- production login works out of the box.
--
-- Portable + idempotent across MySQL / SQLite / Postgres:
--   * INSERT ... SELECT <values> FROM (SELECT 1) AS dummy WHERE NOT EXISTS (...)
--     — `FROM (SELECT 1) AS dummy` is valid on all three (MySQL needs a FROM for
--     a WHERE; SQLite/Postgres accept the derived table too). SQLite-only
--     `INSERT OR IGNORE` and MySQL-only `INSERT IGNORE` are deliberately avoided.
--   * WHERE NOT EXISTS on the primary key makes each insert a no-op if the row is
--     already present, so it never double-inserts alongside the dev sqlite seed.
--   * users.blocked is BOOLEAN — bind FALSE, not 0. Postgres rejects the integer
--     literal ("column blocked is of type boolean but expression is of type integer").
--
-- Admin: admin@admin.com / 12345678
-- password = bcrypt("12345678", rounds=10) — verified against vendor/bcrypt.

INSERT INTO settings (id, initial, name, description, theme, fe_template, created_by, updated_by)
SELECT 'setting-singleton-id-0000000001', 'CA', 'CppAdmin',
       'A C++ Drogon bootstrap admin panel', 'Blue',
       'agency-consulting-002-creative-agency', 'system', 'system'
FROM (SELECT 1) AS dummy
WHERE NOT EXISTS (SELECT 1 FROM settings WHERE id = 'setting-singleton-id-0000000001');

INSERT INTO roles (id, name, guard_name, status, "desc", created_by, updated_by)
SELECT 'role-administrator-000000000001', 'Administrator', 'web', 'Active', '', 'system', 'system'
FROM (SELECT 1) AS dummy
WHERE NOT EXISTS (SELECT 1 FROM roles WHERE id = 'role-administrator-000000000001');

INSERT INTO roles (id, name, guard_name, status, "desc", created_by, updated_by)
SELECT 'role-user-000000000000000000002', 'User', 'web', 'Active', '', 'system', 'system'
FROM (SELECT 1) AS dummy
WHERE NOT EXISTS (SELECT 1 FROM roles WHERE id = 'role-user-000000000000000000002');

INSERT INTO users (id, code, name, phone, email, email_verified_at, password, status, timezone, blocked, blocked_reason, created_by, updated_by)
SELECT 'user-admin-00000000000000000001', '0000000001', 'Administrator', '12345678910',
       'admin@admin.com', CURRENT_TIMESTAMP,
       '$2b$10$abcdefghijklmnopqrstuOxTxdNc3mLA2VsZHWSxlfEcbQuhmajJS',
       'Active', 'Asia/Jakarta', FALSE, '', 'system', 'system'
FROM (SELECT 1) AS dummy
WHERE NOT EXISTS (SELECT 1 FROM users WHERE id = 'user-admin-00000000000000000001');

INSERT INTO users_roles (user_id, role_id)
SELECT 'user-admin-00000000000000000001', 'role-administrator-000000000001'
FROM (SELECT 1) AS dummy
WHERE NOT EXISTS (SELECT 1 FROM users_roles
                 WHERE user_id = 'user-admin-00000000000000000001'
                   AND role_id = 'role-administrator-000000000001');

-- migrate:down
DELETE FROM users_roles WHERE user_id = 'user-admin-00000000000000000001'
                          AND role_id = 'role-administrator-000000000001';
DELETE FROM users    WHERE id = 'user-admin-00000000000000000001';
DELETE FROM roles    WHERE id IN ('role-administrator-000000000001', 'role-user-000000000000000000002');
DELETE FROM settings WHERE id = 'setting-singleton-id-0000000001';
