const prisma = require('../config/prisma');

exports.calculateDailySummary = async (userId, dateString) => {
  const start = new Date(dateString);
  const end = new Date(dateString);
  end.setDate(end.getDate() + 1);

  // Fetch logs for that date
  const logs = await prisma.activityLog.findMany({
    where: {
      userId,
      timestamp: {
        gte: start,
        lt: end
      },
      processed: true
    }
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

  return {
    date: dateString,
    productive_time: productiveTime,
    distracting_time: distractingTime,
    neutral_time: neutralTime,
    learning_time: learningTime,
    total_time: totalTime,
    score: Number(score.toFixed(2))
  };
};
