#!/usr/bin/env bash
# add_ui.sh — Upgrade CppAdmin from API-only to Full (web UI) mode.
# Idempotent: safe to run multiple times. Only copies absent UI files.
# Usage: ./tools/add_ui.sh

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ENV_FILE="$ROOT/.env"

echo "CppAdmin add-ui — upgrading API-only → Full mode"
echo "Root: $ROOT"

# ── 1. Set APP_MODE=full in .env ──────────────────────────────────────────────
if [[ -f "$ENV_FILE" ]]; then
    if grep -q "^APP_MODE=" "$ENV_FILE"; then
        sed -i 's/^APP_MODE=.*/APP_MODE=full/' "$ENV_FILE"
        echo "✓ Updated APP_MODE=full in .env"
    else
        echo "APP_MODE=full" >> "$ENV_FILE"
        echo "✓ Added APP_MODE=full to .env"
    fi
else
    echo "APP_MODE=full" > "$ENV_FILE"
    echo "✓ Created .env with APP_MODE=full"
fi

# ── 2. Check required UI directories exist ────────────────────────────────────
VIEWS_DIR="$ROOT/views/be/default"
LAYOUTS_DIR="$VIEWS_DIR/layouts"

missing=0
for dir in "$VIEWS_DIR/auth" "$LAYOUTS_DIR" "$VIEWS_DIR/dashboard" \
           "$VIEWS_DIR/components" "$VIEWS_DIR/setting" \
           "$VIEWS_DIR/profile" "$VIEWS_DIR/access"; do
    if [[ ! -d "$dir" ]]; then
        echo "  MISSING directory: $dir — run full bootstrap first"
        missing=$((missing+1))
    fi
done

required_views=(
    "$LAYOUTS_DIR/head.csp"
    "$LAYOUTS_DIR/sidebar.csp"
    "$LAYOUTS_DIR/topbar.csp"
    "$LAYOUTS_DIR/foot.csp"
    "$LAYOUTS_DIR/full-width.csp"
    "$VIEWS_DIR/auth/login.csp"
    "$VIEWS_DIR/dashboard/index.csp"
)
for v in "${required_views[@]}"; do
    if [[ ! -f "$v" ]]; then
        echo "  MISSING view: $v"
        missing=$((missing+1))
    fi
done

if [[ $missing -gt 0 ]]; then
    echo ""
    echo "✗ $missing required UI file(s) missing."
    echo "  Run the full bootstrap first, then retry ./tools/add_ui.sh"
    exit 1
fi

# ── 3. Rebuild with ENABLE_WEB_UI=ON ─────────────────────────────────────────
BUILD_DIR="$ROOT/build"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo ""
echo "Configuring cmake with -DENABLE_WEB_UI=ON ..."
cmake .. -DENABLE_WEB_UI=ON \
  -DMYSQL_INCLUDE_DIRS=/usr/include/mysql \
  -DMYSQL_LIBRARIES=/usr/lib/x86_64-linux-gnu/libmysqlclient.so \
  -DHIREDIS_INCLUDE_DIR=/usr/include/hiredis \
  -DHIREDIS_LIBRARY=/usr/lib/x86_64-linux-gnu/libhiredis.so

echo "Building ..."
make -j"$(nproc)"
echo "✓ Build succeeded"

# ── 4. Run convention check ───────────────────────────────────────────────────
echo ""
echo "Running convention checker ..."
cmake --build . --target check_conventions

echo ""
echo "✓ add_ui complete. CppAdmin is now in full (web+api) mode."
echo "  Start: ./build/CppAdmin"
