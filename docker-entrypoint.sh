#!/bin/bash
# CppAdmin container entrypoint (FlazHost / CapRover).
#   1) Rewrite config.json listener to $PORT (CapRover injects PORT).
#   2) Auto-generate + persist SESSION_SECRET/JWT_SECRET (required in production).
#   3) Rewrite db_clients from DB_TYPE/DB_* env (sqlite3 default, mysql/postgres
#      best-effort — Ubuntu's drogon is compiled with all three backends).
#   4) Point drogon's redis client at REDIS_URL, or start a bundled redis-server.
#   5) dbmate up (FATAL on failure → a half-migrated schema must not boot).
#   6) chown the mounted volumes, then exec ./CppAdmin as the unprivileged
#      appuser (PID 1 → clean SIGTERM handling in main.cc).
#
# Runs as root so it can chown CapRover's persistent volumes (which are mounted
# root-owned), then drops privileges with setpriv. The cap_net_bind_service file
# capability on /app/CppAdmin survives the uid switch, so it can still bind :80.
set -u

APP_UID=10001
APP_GID=10001

log()  { echo "[entrypoint] $*"; }
fail() { echo "[entrypoint] FATAL: $*" >&2; exit 1; }

# Numeric guard: `jq --argjson` chokes on an empty/garbage value, which is how a
# `rediss://` URL used to abort the whole config.json rewrite (jq parse error →
# the app silently kept the baked-in port 8010 and localhost redis).
num_or() { case "${1:-}" in ''|*[!0-9]*) echo "$2" ;; *) echo "$1" ;; esac; }
urlenc() { jq -rn --arg v "${1:-}" '$v|@uri'; }

PORT="$(num_or "${PORT:-80}" 80)"
export APP_PORT="$PORT"
export APP_URL="${APP_URL:-http://localhost:${PORT}}"

# ── Secrets: generate once, persist on the /app/data volume ──────────────────
mkdir -p /app/data 2>/dev/null || true
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

# ── Database: drogon db_clients + dbmate DATABASE_URL ────────────────────────
# db/migrations quote the reserved column `desc` the standard way ("desc"), which
# MySQL only understands under sql_mode=ANSI_QUOTES — appended below for dbmate.
# Drogon's own connection can't set sql_mode, so the models emit MySQL backticks
# instead (see include/helpers/SqlIdent.h). Two dialects, one schema.
DB_TYPE="${DB_TYPE:-sqlite3}"
DB_FILE="${DB_FILE:-}"
RDBMS=""
case "$DB_TYPE" in
  sqlite|sqlite3)
    DB_TYPE=sqlite3
    DB_FILE="${DB_FILE:-/app/data/cppadmin.db}"
    mkdir -p "$(dirname "$DB_FILE")" 2>/dev/null || true
    export DATABASE_URL="${DATABASE_URL:-sqlite:${DB_FILE}}"
    ;;
  mysql|postgres|postgresql)
    DB_HOST="${DB_HOST:-127.0.0.1}"
    DB_DATABASE="${DB_DATABASE:-cppadmin}"
    DB_USERNAME="${DB_USERNAME:-root}"
    DB_PASSWORD="${DB_PASSWORD:-}"
    # Credentials go into a URL — percent-encode so a password containing
    # @ : / ? # can't corrupt the DSN.
    enc_user="$(urlenc "$DB_USERNAME")"
    enc_pass="$(urlenc "$DB_PASSWORD")"
    if [ "$DB_TYPE" = "mysql" ]; then
      DB_PORT="$(num_or "${DB_PORT:-3306}" 3306)"
      RDBMS=mysql
      export DATABASE_URL="${DATABASE_URL:-mysql://${enc_user}:${enc_pass}@${DB_HOST}:${DB_PORT}/${DB_DATABASE}}"
      case "$DATABASE_URL" in
        *sql_mode=*) : ;;
        *\?*) DATABASE_URL="${DATABASE_URL}&sql_mode=ANSI_QUOTES" ;;
        *)    DATABASE_URL="${DATABASE_URL}?sql_mode=ANSI_QUOTES" ;;
      esac
      export DATABASE_URL
    else
      DB_PORT="$(num_or "${DB_PORT:-5432}" 5432)"
      RDBMS=postgresql
      export DATABASE_URL="${DATABASE_URL:-postgres://${enc_user}:${enc_pass}@${DB_HOST}:${DB_PORT}/${DB_DATABASE}?sslmode=disable}"
    fi
    ;;
  *)
    log "WARN: unknown DB_TYPE=${DB_TYPE} — leaving config.json db_clients as-is"
    ;;
esac

# ── Redis ────────────────────────────────────────────────────────────────────
# Drogon 1.8.7 has NO TLS support in its redis client, so a managed `rediss://`
# endpoint is unreachable. Rather than crash-loop on it, fall back to the bundled
# local redis-server: RateLimitFilter then rate-limits per container instead of
# fleet-wide. REDIS_URL=disabled removes the redis client entirely.
REDIS_URL="${REDIS_URL:-redis://127.0.0.1:6379}"
REDIS_MODE=on
REDIS_HOST=""; REDIS_PORT=6379; REDIS_PASSWORD=""; REDIS_USERNAME=""; REDIS_DB=0

parse_redis_url() {
  local u="${1#*://}"
  case "$u" in */*) REDIS_DB="$(num_or "${u##*/}" 0)"; u="${u%%/*}" ;; esac
  case "$u" in
    *@*) local ui="${u%@*}"; u="${u##*@}"
         case "$ui" in
           *:*) REDIS_USERNAME="${ui%%:*}"; REDIS_PASSWORD="${ui#*:}" ;;
           *)   REDIS_USERNAME="$ui" ;;
         esac ;;
  esac
  REDIS_HOST="${u%%:*}"
  case "$u" in *:*) REDIS_PORT="$(num_or "${u##*:}" 6379)" ;; *) REDIS_PORT=6379 ;; esac
}

case "$REDIS_URL" in
  disabled|"")
    REDIS_MODE=off
    log "redis: disabled (RateLimitFilter fails open)"
    ;;
  rediss://*)
    parse_redis_url "$REDIS_URL"
    log "WARN: REDIS_URL uses TLS (rediss://${REDIS_HOST}:${REDIS_PORT}) but drogon's"
    log "WARN: redis client has no TLS support — falling back to bundled local redis."
    REDIS_HOST=127.0.0.1; REDIS_PORT=6379; REDIS_PASSWORD=""; REDIS_USERNAME=""; REDIS_DB=0
    ;;
  redis://*)
    parse_redis_url "$REDIS_URL"
    ;;
  *)
    log "WARN: unrecognised REDIS_URL scheme — treating as host[:port]"
    parse_redis_url "redis://$REDIS_URL"
    ;;
esac

if [ "$REDIS_MODE" = "on" ] &&
   { [ "$REDIS_HOST" = "127.0.0.1" ] || [ "$REDIS_HOST" = "localhost" ]; }; then
  log "starting bundled redis-server on 127.0.0.1:${REDIS_PORT}"
  setpriv --reuid="$APP_UID" --regid="$APP_GID" --clear-groups \
    redis-server --port "$REDIS_PORT" --bind 127.0.0.1 \
                 --save '' --appendonly no --dir /app/data \
                 --daemonize no >/dev/null 2>&1 &
  for _ in $(seq 1 25); do
    redis-cli -p "$REDIS_PORT" ping >/dev/null 2>&1 && break
    sleep 0.2
  done
fi

# ── Rewrite config.json: listener port + db + redis ──────────────────────────
# Every value is passed as a jq --arg/--argjson binding. Interpolating them into
# the jq *program* (as this script used to) breaks on empty values and on
# passwords containing a quote or backslash.
log "rewriting config.json (listen 0.0.0.0:${PORT}, db=${DB_TYPE}, redis=${REDIS_MODE})"
if jq \
      --argjson port      "$PORT" \
      --arg     dbtype    "$DB_TYPE" \
      --arg     dbfile    "$DB_FILE" \
      --arg     rdbms     "$RDBMS" \
      --arg     dbhost    "${DB_HOST:-}" \
      --argjson dbport    "$(num_or "${DB_PORT:-0}" 0)" \
      --arg     dbname    "${DB_DATABASE:-}" \
      --arg     dbuser    "${DB_USERNAME:-}" \
      --arg     dbpass    "${DB_PASSWORD:-}" \
      --arg     redismode "$REDIS_MODE" \
      --arg     redishost "$REDIS_HOST" \
      --argjson redisport "$REDIS_PORT" \
      --arg     redisuser "$REDIS_USERNAME" \
      --arg     redispass "$REDIS_PASSWORD" \
      --argjson redisdb   "$REDIS_DB" \
      '
      .listeners = [{address:"0.0.0.0", port:$port, https:false}]
      | .db_clients = (
          if   $dbtype == "sqlite3"
          then [{name:"default", rdbms:"sqlite3", filename:$dbfile,
                 number_of_connections:1, timeout:10.0}]
          elif $rdbms != ""
          then [{name:"default", rdbms:$rdbms, host:$dbhost, port:$dbport,
                 dbname:$dbname, user:$dbuser, passwd:$dbpass,
                 number_of_connections:2, timeout:10.0}]
          else .db_clients
          end)
      | if   $redismode == "off"
        then del(.redis_clients)
        else .redis_clients = [{name:"default", host:$redishost, port:$redisport,
                                username:$redisuser, passwd:$redispass, db:$redisdb,
                                is_fast:false, number_of_connections:2, timeout:5.0}]
        end
      ' \
      /app/config.json > /app/config.json.new; then
  mv /app/config.json.new /app/config.json
else
  rm -f /app/config.json.new
  fail "jq rewrite of config.json failed — refusing to boot on the baked-in defaults"
fi

# ── Migrations (dbmate) ──────────────────────────────────────────────────────
# Fatal on failure: booting on a half-created schema is what turned a one-line
# SQL incompatibility into a silent 500-on-every-page deploy.
log "running migrations: dbmate up (DATABASE_URL=${DATABASE_URL%%\?*})"
# Bounded so an unreachable DB fails the boot fast instead of hanging the
# container at 90% forever (dbmate's driver has no built-in connect deadline).
if timeout "${MIGRATE_TIMEOUT:-120}" \
     dbmate --no-dump-schema --migrations-dir /app/db/migrations up; then
  log "migrations OK"
else
  rc=$?
  [ "$rc" = 124 ] && fail "dbmate up timed out after ${MIGRATE_TIMEOUT:-120}s — DB unreachable?"
  fail "dbmate up failed (exit $rc) — check DB connectivity/credentials and the log above"
fi

# ── Seed (idempotent INSERT OR IGNORE — sqlite only) ─────────────────────────
# Non-sqlite is seeded by db/migrations/20260710000003_seed_admin.sql instead.
if [ "$DB_TYPE" = "sqlite3" ] && [ -f /app/db/seeds/seed.sql ]; then
  if sqlite3 "$DB_FILE" < /app/db/seeds/seed.sql; then
    log "seed OK (admin@admin.com / 12345678)"
  else
    log "WARN: seed failed — continuing"
  fi
fi

# ── Hand the runtime dirs to appuser, then drop privileges ───────────────────
# CapRover mounts persistent volumes root-owned; drogon creates its upload temp
# tree (storage/uploads/tmp/00..FF) on startup and FeCatalogService writes
# storage/fe, so both must belong to the user that runs the server.
mkdir -p /app/storage/uploads /app/storage/fe 2>/dev/null || true
chown -R "${APP_UID}:${APP_GID}" /app/data /app/storage /app/config.json 2>/dev/null || \
  log "WARN: could not chown /app/data + /app/storage — uploads may fail"

log "starting CppAdmin on 0.0.0.0:${PORT} (APP_ENV=${APP_ENV:-production}, APP_MODE=${APP_MODE:-full})"
exec setpriv --reuid="$APP_UID" --regid="$APP_GID" --clear-groups /app/CppAdmin
