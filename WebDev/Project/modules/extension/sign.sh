#!/bin/bash
# ─────────────────────────────────────────
# Sign & package extension for Firefox
# ─────────────────────────────────────────
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Load API keys from .env
if [ -f .env ]; then
  export $(grep -v '^#' .env | xargs)
else
  echo "❌ Missing .env file with WEB_EXT_API_KEY and WEB_EXT_API_SECRET"
  exit 1
fi

if [ -z "$WEB_EXT_API_KEY" ] || [ -z "$WEB_EXT_API_SECRET" ]; then
  echo "❌ WEB_EXT_API_KEY and WEB_EXT_API_SECRET must be set in .env"
  exit 1
fi

echo "📦 Signing extension (unlisted/private)..."
npx web-ext sign \
  --source-dir=. \
  --api-key="$WEB_EXT_API_KEY" \
  --api-secret="$WEB_EXT_API_SECRET" \
  --channel=unlisted \
  --ignore-files=".env" "sign.sh" "manifest_chrome.json" "web-ext-artifacts"

echo ""
echo "✅ Done! Your signed .xpi is in: web-ext-artifacts/"
echo "📥 Install: Firefox → about:addons → ⚙️ → Install Add-on From File"
