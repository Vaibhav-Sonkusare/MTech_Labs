const prisma = require('../config/prisma');

/**
 * Calculate daily summary with DB caching.
 *
 * For past dates: checks cache first, returns if found.
 * For today: always recomputes (data may still be incoming).
 * Caches results for past dates after computing.
 */
exports.calculateDailySummary = async (userId, dateString) => {
  const today = new Date().toISOString().split('T')[0];
  const isPastDate = dateString < today;

  // Check cache for past dates
  if (isPastDate) {
    const cached = await prisma.dailySummary.findUnique({
      where: {
        userId_date: {
          userId,
          date: new Date(dateString),
        },
      },
    });

    if (cached) {
      return {
        date: dateString,
        productive_time: cached.productiveTime,
        distracting_time: cached.distractingTime,
        neutral_time: cached.neutralTime,
        learning_time: cached.learningTime,
        total_time: cached.totalTime,
        score: cached.score,
      };
    }
  }

  // Compute from logs
  const start = new Date(dateString);
  const end = new Date(dateString);
  end.setDate(end.getDate() + 1);

  const logs = await prisma.activityLog.findMany({
    where: {
      userId,
      timestamp: { gte: start, lt: end },
      processed: true,
    },
  });

  let productiveTime = 0;
  let distractingTime = 0;
  let neutralTime = 0;
  let learningTime = 0;

  logs.forEach(log => {
    switch (log.category) {
      case 'productive':
        productiveTime += log.durationSeconds;
        break;
      case 'distracting':
        distractingTime += log.durationSeconds;
        break;
      case 'learning':
        learningTime += log.durationSeconds;
        break;
      default:
        neutralTime += log.durationSeconds;
    }
  });

  const totalTime =
    productiveTime + distractingTime + neutralTime + learningTime;

  const score =
    totalTime === 0
      ? 0
      : productiveTime / totalTime;

  const result = {
    date: dateString,
    productive_time: productiveTime,
    distracting_time: distractingTime,
    neutral_time: neutralTime,
    learning_time: learningTime,
    total_time: totalTime,
    score: Number(score.toFixed(2)),
  };

  // Cache past dates (upsert in case of race conditions)
  if (isPastDate && totalTime > 0) {
    try {
      await prisma.dailySummary.upsert({
        where: {
          userId_date: { userId, date: new Date(dateString) },
        },
        update: {
          productiveTime,
          distractingTime,
          neutralTime,
          learningTime,
          totalTime,
          score: result.score,
          computedAt: new Date(),
        },
        create: {
          userId,
          date: new Date(dateString),
          productiveTime,
          distractingTime,
          neutralTime,
          learningTime,
          totalTime,
          score: result.score,
        },
      });
    } catch (err) {
      // Don't fail the request if caching fails
      console.error('Failed to cache daily summary:', err.message);
    }
  }

  return result;
};
