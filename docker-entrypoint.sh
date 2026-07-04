#!/bin/bash
# CppAdmin container entrypoint (FlazHost / CapRover).
#   1) Rewrite config.json listener to $PORT (CapRover injects PORT; the app's
#      listen port comes ONLY from config.json — main.cc does not apply APP_PORT).
#   2) Auto-generate + persist SESSION_SECRET/JWT_SECRET (required in production).
#   3) Rewrite db_clients from DB_TYPE/DB_* env (sqlite3 default, mysql/postgres
#      best-effort — Ubuntu's drogon is compiled with all three backends).
#   4) Start bundled redis-server (RateLimitFilter; fails open if unavailable).
#   5) dbmate up (WARN-and-continue on failure) → idempotent sqlite seed.
#   6) exec ./CppAdmin (PID 1 → clean SIGTERM handling in main.cc).
set -u

log() { echo "[entrypoint] $*"; }

PORT="${PORT:-80}"
export APP_PORT="$PORT"
export APP_URL="${APP_URL:-http://localhost:${PORT}}"

# ── Secrets: generate once, persist on the /app/data volume ──────────────────
SECRETS_FILE=/app/data/.runtime-secrets
if [ -z "${SESSION_SECRET:-}" ] || [ -z "${JWT_SECRET:-}" ]; then
  if [ ! -f "$SECRETS_FILE" ]; then
    log "generating runtime secrets -> $SECRETS_FILE"
    ( umask 077
      {
        echo "SESSION_SECRET=$(openssl rand -hex 32)"
        echo "JWT_SECRET=$(openssl rand -hex 32)"
      } > "$SECRETS_FILE"
    )
  fi
  gen_session="$(grep '^SESSION_SECRET=' "$SECRETS_FILE" | head -1 | cut -d= -f2-)"
  gen_jwt="$(grep '^JWT_SECRET=' "$SECRETS_FILE" | head -1 | cut -d= -f2-)"
  export SESSION_SECRET="${SESSION_SECRET:-$gen_session}"
  export JWT_SECRET="${JWT_SECRET:-$gen_jwt}"
fi

# ── Database: build the drogon db_clients block + dbmate DATABASE_URL ────────
DB_TYPE="${DB_TYPE:-sqlite3}"
case "$DB_TYPE" in
  sqlite|sqlite3)
    DB_TYPE=sqlite3
    DB_FILE="${DB_FILE:-/app/data/cppadmin.db}"
    mkdir -p "$(dirname "$DB_FILE")" 2>/dev/null || true
    export DATABASE_URL="${DATABASE_URL:-sqlite:${DB_FILE}}"
    DB_JQ=".db_clients = [{name:\"default\", rdbms:\"sqlite3\", filename:\$dbfile,
             number_of_connections:1, timeout:10.0}]"
    ;;
  mysql|postgres|postgresql)
    DB_HOST="${DB_HOST:-127.0.0.1}"
    DB_DATABASE="${DB_DATABASE:-cppadmin}"
    DB_USERNAME="${DB_USERNAME:-root}"
    DB_PASSWORD="${DB_PASSWORD:-}"
    if [ "$DB_TYPE" = "mysql" ]; then
      DB_PORT="${DB_PORT:-3306}"
      RDBMS=mysql
      export DATABASE_URL="${DATABASE_URL:-mysql://${DB_USERNAME}:${DB_PASSWORD}@${DB_HOST}:${DB_PORT}/${DB_DATABASE}}"
    else
      DB_PORT="${DB_PORT:-5432}"
      RDBMS=postgresql
      export DATABASE_URL="${DATABASE_URL:-postgres://${DB_USERNAME}:${DB_PASSWORD}@${DB_HOST}:${DB_PORT}/${DB_DATABASE}?sslmode=disable}"
    fi
    DB_JQ=".db_clients = [{name:\"default\", rdbms:\"${RDBMS}\", host:\"${DB_HOST}\",
             port:${DB_PORT}, dbname:\"${DB_DATABASE}\", user:\"${DB_USERNAME}\",
             passwd:\"${DB_PASSWORD}\", number_of_connections:2, timeout:10.0}]"
    ;;
  *)
    log "WARN: unknown DB_TYPE=${DB_TYPE} — leaving config.json db_clients as-is"
    DB_JQ="."
    ;;
esac

# ── Redis: bundled local redis-server by default; external via REDIS_URL ─────
# RateLimitFilter fails open without redis, but drogon's client log-spams
# reconnects, so we bundle a tiny local instance. REDIS_URL=disabled strips it.
REDIS_URL="${REDIS_URL:-redis://127.0.0.1:6379}"
if [ "$REDIS_URL" = "disabled" ] || [ -z "$REDIS_URL" ]; then
  REDIS_JQ="del(.redis_clients)"
  REDIS_HOST=""
else
  hostport="${REDIS_URL#redis://}"; hostport="${hostport%%/*}"
  hostport="${hostport##*@}"                      # drop user:pass@ if present
  REDIS_HOST="${hostport%%:*}"
  REDIS_PORT="${hostport##*:}"
  [ "$REDIS_PORT" = "$REDIS_HOST" ] && REDIS_PORT=6379
  REDIS_JQ=".redis_clients[0].host = \"${REDIS_HOST}\" | .redis_clients[0].port = ${REDIS_PORT}"
fi

if [ "$REDIS_HOST" = "127.0.0.1" ] || [ "$REDIS_HOST" = "localhost" ]; then
  log "starting bundled redis-server on 127.0.0.1:${REDIS_PORT}"
  redis-server --port "$REDIS_PORT" --bind 127.0.0.1 \
               --save '' --appendonly no --dir /app/data \
               --daemonize no >/dev/null 2>&1 &
  for _ in $(seq 1 25); do
    redis-cli -p "$REDIS_PORT" ping >/dev/null 2>&1 && break
    sleep 0.2
  done
fi

# ── Rewrite config.json: listener port + db + redis ──────────────────────────
log "rewriting config.json (listen 0.0.0.0:${PORT}, db=${DB_TYPE})"
if jq --arg dbfile "${DB_FILE:-}" \
      ".listeners = [{address:\"0.0.0.0\", port:${PORT}, https:false}]
       | ${DB_JQ}
       | ${REDIS_JQ}" \
      /app/config.json > /app/config.json.new; then
  mv /app/config.json.new /app/config.json
else
  log "WARN: jq rewrite failed — starting with original config.json (port 8010!)"
  rm -f /app/config.json.new
fi

# ── Migrations (dbmate) — failure must NOT block boot ─────────────────────────
log "running migrations: dbmate up (DATABASE_URL=${DATABASE_URL%%\?*})"
if dbmate --no-dump-schema --migrations-dir /app/db/migrations up; then
  log "migrations OK"
else
  log "WARN: dbmate up failed — continuing (existing schema may still work)"
fi

# ── Seed (idempotent INSERT OR IGNORE — sqlite only) ─────────────────────────
if [ "$DB_TYPE" = "sqlite3" ] && [ -f /app/db/seeds/seed.sql ]; then
  if sqlite3 "$DB_FILE" < /app/db/seeds/seed.sql; then
    log "seed OK (admin@admin.com / 12345678)"
  else
    log "WARN: seed failed — continuing"
  fi
elif [ "$DB_TYPE" != "sqlite3" ]; then
  log "WARN: seed.sql uses sqlite INSERT OR IGNORE — skipped for ${DB_TYPE}; seed manually"
fi

# ── Start the server (binary at /app/CppAdmin → appRoot=/app in main.cc) ─────
log "starting CppAdmin on 0.0.0.0:${PORT} (APP_ENV=${APP_ENV:-production}, APP_MODE=${APP_MODE:-full})"
exec /app/CppAdmin
