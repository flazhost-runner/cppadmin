CREATE TABLE IF NOT EXISTS "schema_migrations" (version varchar(128) primary key);
CREATE TABLE users (
    id                  VARCHAR(36)  NOT NULL PRIMARY KEY,
    code                VARCHAR(20)  NOT NULL,
    name                VARCHAR(50)  NOT NULL,
    phone               VARCHAR(15),
    email               VARCHAR(255) NOT NULL,
    email_verified_at   TIMESTAMP,
    password            VARCHAR(255) NOT NULL,
    password_otp        VARCHAR(255),
    password_otp_expires BIGINT,
    status              VARCHAR(20)  NOT NULL DEFAULT 'Active',
    picture             VARCHAR(255),
    blocked             BOOLEAN      NOT NULL DEFAULT FALSE,
    blocked_reason      VARCHAR(255),
    timezone            VARCHAR(255) NOT NULL DEFAULT 'UTC',
    created_by          VARCHAR(36),
    updated_by          VARCHAR(36),
    created_at          TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at          TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE UNIQUE INDEX users__code  ON users(code);
CREATE UNIQUE INDEX users__email ON users(email);
CREATE TABLE roles (
    id          VARCHAR(36)  NOT NULL PRIMARY KEY,
    name        VARCHAR(255) NOT NULL,
    guard_name  VARCHAR(20)  NOT NULL DEFAULT 'web',
    status      VARCHAR(20)  NOT NULL DEFAULT 'Active',
    "desc"      VARCHAR(255),
    created_by  VARCHAR(36),
    updated_by  VARCHAR(36),
    created_at  TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at  TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE UNIQUE INDEX roles__name ON roles(name);
CREATE TABLE permissions (
    id          VARCHAR(36)  NOT NULL PRIMARY KEY,
    name        VARCHAR(255) NOT NULL,
    guard_name  VARCHAR(20)  NOT NULL DEFAULT 'web',
    method      VARCHAR(255),
    status      VARCHAR(20)  NOT NULL DEFAULT 'Active',
    "desc"      VARCHAR(255),
    created_by  VARCHAR(36),
    updated_by  VARCHAR(36),
    created_at  TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at  TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX permissions__name       ON permissions(name);
CREATE INDEX permissions__guard_name ON permissions(guard_name);
CREATE TABLE settings (
    id          VARCHAR(36)  NOT NULL PRIMARY KEY,
    initial     VARCHAR(255),
    name        VARCHAR(255),
    description TEXT,
    icon        VARCHAR(255),
    logo        VARCHAR(255),
    favicon     VARCHAR(255),
    login_image VARCHAR(255),
    phone       VARCHAR(255),
    address     VARCHAR(255),
    email       VARCHAR(255),
    copyright   VARCHAR(255),
    theme       VARCHAR(20)  NOT NULL DEFAULT 'Blue',
    fe_template VARCHAR(80)  NOT NULL DEFAULT 'agency-consulting-002-creative-agency',
    created_by  VARCHAR(36),
    updated_by  VARCHAR(36),
    created_at  TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at  TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE TABLE users_roles (
    user_id  VARCHAR(36) NOT NULL,
    role_id  VARCHAR(36) NOT NULL,
    PRIMARY KEY (user_id, role_id),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (role_id) REFERENCES roles(id) ON DELETE CASCADE
);
CREATE TABLE roles_permissions (
    role_id       VARCHAR(36) NOT NULL,
    permission_id VARCHAR(36) NOT NULL,
    PRIMARY KEY (role_id, permission_id),
    FOREIGN KEY (role_id)       REFERENCES roles(id)       ON DELETE CASCADE,
    FOREIGN KEY (permission_id) REFERENCES permissions(id) ON DELETE CASCADE
);
CREATE TABLE jwt_blacklist (
    jti        VARCHAR(36)  NOT NULL PRIMARY KEY,
    expires_at TIMESTAMP    NOT NULL
);
CREATE INDEX idx_jwt_blacklist_exp ON jwt_blacklist(expires_at);
-- Dbmate schema migrations
INSERT INTO "schema_migrations" (version) VALUES
  ('20240101000001'),
  ('20260630000002'),
  ('20260710000003');
