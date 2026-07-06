# Dev container CppAdmin (Drogon) — toolchain + seluruh dependensi Drogon siap
# pakai, sehingga "Reopen in Container" langsung bisa build.
#
# PENTING: pakai libmariadb-dev + libmariadb-dev-compat (BUKAN default-
# libmysqlclient-dev): di Ubuntu 24.04 bersih, FindMySQL.cmake milik Drogon
# mencari lib bernama `mariadbclient`/`mysqlclient_r`; default-libmysqlclient-dev
# TIDAK menyediakannya (cmake gagal "Cannot find MySQL" — terverifikasi).
# Paket `drogon` (CLI drogon_ctl) WAJIB untuk meng-compile CSP views.
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
      cmake make g++ pkg-config \
      drogon libdrogon-dev \
      libjsoncpp-dev uuid-dev zlib1g-dev \
      libyaml-cpp-dev libbrotli-dev libpq-dev \
      libssl-dev libcurl4-openssl-dev \
      libjwt-dev libhiredis-dev libsqlite3-dev \
      libmariadb-dev libmariadb-dev-compat \
      sqlite3 curl ca-certificates git python3 gdb \
 && rm -rf /var/lib/apt/lists/*

# dbmate — runner migrasi SQL yang dipakai CppAdmin (db/migrations).
RUN curl -fsSL -o /usr/local/bin/dbmate \
      https://github.com/amacneil/dbmate/releases/latest/download/dbmate-linux-amd64 \
 && chmod +x /usr/local/bin/dbmate
