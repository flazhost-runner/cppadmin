-- Idempotent seed — runs on every `dev` startup; safe to run multiple times.
-- Admin user: admin@admin.com / 12345678 (bcrypt hash for "12345678", rounds=10)
-- Matches NodeAdmin seed exactly.

INSERT OR IGNORE INTO settings (id, initial, name, description, theme, fe_template, created_by, updated_by)
VALUES (
    'setting-singleton-id-0000000001',
    'CA',
    'CppAdmin',
    'A C++ Drogon bootstrap admin panel',
    'Blue',
    'agency-consulting-002-creative-agency',
    'system',
    'system'
);

INSERT OR IGNORE INTO roles (id, name, guard_name, status, desc, created_by, updated_by)
VALUES
    ('role-administrator-000000000001', 'Administrator', 'web', 'Active', '', 'system', 'system'),
    ('role-user-000000000000000000002', 'User',          'web', 'Active', '', 'system', 'system');

-- bcrypt hash of "12345678" with rounds=10 (generated via crypt_r for C bcrypt compatibility)
-- $2b$10$abcdefghijklmnopqrstuOxTxdNc3mLA2VsZHWSxlfEcbQuhmajJS
INSERT OR IGNORE INTO users (id, code, name, phone, email, email_verified_at, password, status, timezone, blocked, blocked_reason, created_by, updated_by)
VALUES (
    'user-admin-00000000000000000001',
    '0000000001',
    'Administrator',
    '12345678910',
    'admin@admin.com',
    CURRENT_TIMESTAMP,
    '$2b$10$abcdefghijklmnopqrstuOxTxdNc3mLA2VsZHWSxlfEcbQuhmajJS',
    'Active',
    'Asia/Jakarta',
    0,
    '',
    'system',
    'system'
);

INSERT OR IGNORE INTO users_roles (user_id, role_id)
VALUES ('user-admin-00000000000000000001', 'role-administrator-000000000001');
