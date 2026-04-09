const prisma = require('../config/prisma');
const dailyAnalytics = require('./dailyAnalytics');

/**
 * Calculate weekly summary with per-day breakdown.
 *
 * @param {string} userId
 * @param {string} dateString - End date (YYYY-MM-DD). Computes 7 days ending on this date.
 * @returns Weekly summary with daily trend array.
 */
exports.calculateWeeklySummary = async (userId, dateString) => {
  const endDate = new Date(dateString);
  const startDate = new Date(dateString);
  startDate.setDate(startDate.getDate() - 6); // 7 days including endDate

  // Build per-day breakdown by reusing daily analytics
  const dailyBreakdown = [];

  for (let i = 0; i < 7; i++) {
    const d = new Date(startDate);
    d.setDate(d.getDate() + i);
    const ds = d.toISOString().split('T')[0];

    const summary = await dailyAnalytics.calculateDailySummary(userId, ds);
    dailyBreakdown.push(summary);
  }

  // Aggregate totals across the week
  let productiveTime = 0;
  let distractingTime = 0;
  let neutralTime = 0;
  let learningTime = 0;

  dailyBreakdown.forEach(day => {
    productiveTime += day.productive_time;
    distractingTime += day.distracting_time;
    neutralTime += day.neutral_time;
    learningTime += day.learning_time;
  });

  const totalTime =
    productiveTime + distractingTime + neutralTime + learningTime;

  const score =
    totalTime === 0
      ? 0
      : productiveTime / totalTime;

  return {
    start_date: startDate.toISOString().split('T')[0],
    end_date: dateString,
    productive_time: productiveTime,
    distracting_time: distractingTime,
    neutral_time: neutralTime,
    learning_time: learningTime,
    total_time: totalTime,
    score: Number(score.toFixed(2)),
    daily_breakdown: dailyBreakdown
  };
};
