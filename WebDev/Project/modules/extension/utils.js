export function extractDomain(url) {
  try {
    const parsed = new URL(url);
    return parsed.hostname;
  } catch {
    return null;
  }
}

export function nowISO() {
  return new Date().toISOString();
}

export function durationSeconds(startTime) {
  return Math.floor((Date.now() - startTime) / 1000);
}
