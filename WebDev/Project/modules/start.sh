#!/bin/bash
# ─────────────────────────────────────────────────
# Smart Digital Wellbeing — Single-Process Launcher
# ─────────────────────────────────────────────────
#
# Starts the unified Node.js backend that serves:
#   - REST API (authentication, activity, analytics)
#   - AI classifier (built-in, no Python needed)
#   - Dashboard (static files)
#   - SQLite database (file-based, no server needed)
#
# Usage:
#   ./start.sh          — normal start
#   ./start.sh --build  — rebuild dashboard first, then start
#

set -e

# Resolve the directory this script lives in (works even via symlinks)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BACKEND_DIR="$SCRIPT_DIR/backend"
DASHBOARD_DIR="$SCRIPT_DIR/dashboard"

# Ensure data directory exists for SQLite
mkdir -p "$BACKEND_DIR/data"

# Rebuild dashboard if --build flag is passed
if [[ "$1" == "--build" ]]; then
  echo "📦 Building dashboard..."
  cd "$DASHBOARD_DIR"
  npm run build
  echo "✅ Dashboard built"
fi

# Check if dashboard is built
if [ ! -f "$DASHBOARD_DIR/dist/index.html" ]; then
  echo "⚠️  Dashboard not built. Building now..."
  cd "$DASHBOARD_DIR"
  npm run build
  echo "✅ Dashboard built"
fi

# Start the backend (serves everything)
echo "🚀 Starting Wellbeing Platform..."
cd "$BACKEND_DIR"
exec node server.js
