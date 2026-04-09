const prisma = require('../config/prisma');

/**
 * Calculate productive streak (consecutive days with score >= 0.4).
 */
async function getStreak(userId) {
  const summaries = await prisma.dailySummary.findMany({
    where: { userId },
    orderBy: { date: 'desc' },
    select: { date: true, score: true },
  });

  if (summaries.length === 0) {
    return { currentStreak: 0, longestStreak: 0, lastActiveDate: null };
  }

  // Calculate current streak (from today backwards)
  let currentStreak = 0;
  const today = new Date();
  today.setHours(0, 0, 0, 0);

  for (const summary of summaries) {
    const summaryDate = new Date(summary.date);
    summaryDate.setHours(0, 0, 0, 0);

    // Check if this is the expected date in the streak
    const expectedDate = new Date(today);
    expectedDate.setDate(expectedDate.getDate() - currentStreak);
    expectedDate.setHours(0, 0, 0, 0);

    // Allow 1 day gap for "today not yet recorded"
    if (currentStreak === 0) {
      const diffDays = Math.floor((today - summaryDate) / 86400000);
      if (diffDays > 1) break;
    } else {
      if (summaryDate.getTime() !== expectedDate.getTime()) break;
    }

    if (summary.score >= 0.4) {
      currentStreak++;
    } else {
      break;
    }
  }

  // Calculate longest streak
  let longestStreak = 0;
  let streak = 0;
  // Sort ascending for longest calculation
  const sorted = [...summaries].reverse();
  for (let i = 0; i < sorted.length; i++) {
    if (sorted[i].score >= 0.4) {
      streak++;
      if (i > 0) {
        const prev = new Date(sorted[i - 1].date);
        const curr = new Date(sorted[i].date);
        const diff = Math.floor((curr - prev) / 86400000);
        if (diff > 1) streak = 1; // Gap detected, restart
      }
      longestStreak = Math.max(longestStreak, streak);
    } else {
      streak = 0;
    }
  }

  return {
    currentStreak,
    longestStreak,
    lastActiveDate: summaries[0]?.date || null,
  };
}

module.exports = { getStreak };
