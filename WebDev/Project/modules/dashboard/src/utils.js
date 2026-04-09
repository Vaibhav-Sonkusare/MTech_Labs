/**
 * Format seconds into human-readable time.
 * e.g. 3661 → "1h 1m"
 */
export function formatTime(seconds) {
  if (!seconds || seconds === 0) return '0m';

  const hours = Math.floor(seconds / 3600);
  const mins = Math.floor((seconds % 3600) / 60);

  if (hours === 0) return `${mins}m`;
  if (mins === 0) return `${hours}h`;
  return `${hours}h ${mins}m`;
}

/**
 * Format hour number to readable time.
 * e.g. 14 → "2 PM"
 */
export function formatHour(hour) {
  if (hour === 0) return '12 AM';
  if (hour === 12) return '12 PM';
  if (hour < 12) return `${hour} AM`;
  return `${hour - 12} PM`;
}

/**
 * Get color for a category.
 */
export function getCategoryColor(category) {
  const colors = {
    productive: '#22c55e',
    distracting: '#ef4444',
    learning: '#8b5cf6',
    neutral: '#64748b',
  };
  return colors[category] || colors.neutral;
}

/**
 * Get today's date as YYYY-MM-DD.
 */
export function getToday() {
  return new Date().toISOString().split('T')[0];
}
