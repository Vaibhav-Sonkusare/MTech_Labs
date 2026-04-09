/**
 * Browser API Polyfill
 *
 * Chrome uses `chrome.*` APIs while Firefox uses `browser.*`.
 * This shim ensures `browser.*` is available in Chrome by aliasing it.
 *
 * For Firefox: `browser` is already defined natively, so this is a no-op.
 * For Chrome:  `browser` is undefined, so we alias `chrome` as `browser`.
 */
if (typeof globalThis.browser === 'undefined') {
  globalThis.browser = chrome;
}
