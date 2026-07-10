# syntax=docker/dockerfile:1
# ── CppAdmin starter kit · FlazHost PaaS (CapRover) ──────────────────────────
# Multi-stage build. Drogon 1.8.7 comes from Ubuntu 24.04 apt (libdrogon-dev),
# compiled with sqlite3 + mysql (mariadb) + postgres + redis (hiredis) support.
# CSP views are compiled into the binary at build time (drogon_create_views).

# 1) Build stage
FROM ubuntu:24.04 AS build
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
      g++ cmake make pkg-config ca-certificates curl \
      drogon libdrogon-dev libjsoncpp-dev uuid-dev zlib1g-dev libbrotli-dev \
      libssl-dev libsqlite3-dev libhiredis-dev libyaml-cpp-dev \
      libmariadb-dev libpq-dev libcurl4-openssl-dev \
 && rm -rf /var/lib/apt/lists/*

# dbmate — static binary used at runtime for SQL migrations (db/migrations).
ARG DBMATE_VERSION=v2.19.0
RUN arch="$(dpkg --print-architecture)" \
 && curl -fsSL -o /usr/local/bin/dbmate \
      "https://github.com/amacneil/dbmate/releases/download/${DBMATE_VERSION}/dbmate-linux-${arch}" \
 && chmod +x /usr/local/bin/dbmate

WORKDIR /src
COPY . .
# Build only the server target (skips test binary; views compiled into binary).
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
 && cmake --build build --target CppAdmin -j"$(nproc)"

# 2) Runtime stage — same family (ubuntu:24.04), shared libs only.
FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
# libdrogon1t64 pulls all drogon deps (jsoncpp, hiredis, mariadb, pq, sqlite3,
# uuid, yaml-cpp, brotli, zlib, trantor). sqlite3 CLI = seed; jq = config.json
# rewrite; openssl = secret generation; redis-server bundled for RateLimitFilter.
# setpriv (util-linux, already in the base image) drops privileges in the entrypoint.
RUN apt-get update && apt-get install -y --no-install-recommends \
      libdrogon1t64 libcurl4t64 sqlite3 openssl jq redis-server \
      ca-certificates tzdata libcap2-bin \
 && rm -rf /var/lib/apt/lists/* \
 && groupadd -g 10001 appuser \
 && useradd -r -u 10001 -g 10001 -d /app appuser

WORKDIR /app
COPY --from=build /src/build/CppAdmin   /app/CppAdmin
COPY --from=build /usr/local/bin/dbmate /usr/local/bin/dbmate

# Runtime assets: config.json (listeners/db/redis — rewritten by entrypoint),
# public/ (static files, main.cc sets documentRoot = <appRoot>/public),
# db/ (dbmate migrations + idempotent seed).
COPY config.json          /app/config.json
COPY public               /app/public
COPY db                   /app/db
COPY docker-entrypoint.sh /app/docker-entrypoint.sh

RUN chmod +x /app/docker-entrypoint.sh \
 && mkdir -p /app/data /app/storage/uploads /app/storage/fe \
 && chown -R appuser:appuser /app \
 # Allow the non-root user to bind the privileged port 80 (CapRover default).
 # The file capability is granted at execve regardless of the calling uid, so it
 # survives the entrypoint's setpriv drop to appuser.
 && setcap 'cap_net_bind_service=+ep' /app/CppAdmin

# ── Zero-config defaults (all overridable via env) ──────────────────────────
# APP_ENV=production → main.cc skips dev auto-migrate (entrypoint handles it)
# and AppConfig requires SESSION_SECRET/JWT_SECRET (entrypoint auto-generates
# + persists them in /app/data/.runtime-secrets when not provided).
ENV APP_ENV=production \
    APP_MODE=full \
    APP_NAME=CppAdmin \
    PORT=80 \
    DB_TYPE=sqlite3 \
    DB_FILE=/app/data/cppadmin.db \
    REDIS_URL=redis://127.0.0.1:6379

# No `USER appuser` here: the entrypoint needs root to chown CapRover's
# root-owned persistent volumes (/app/data, /app/storage), then execs the server
# as uid 10001 via setpriv. The server itself never runs as root.
EXPOSE 80
ENTRYPOINT ["/app/docker-entrypoint.sh"]
