const prisma = require('../config/prisma');

/**
 * Get hourly activity breakdown and identify peak productive hour.
 *
 * @param {string} userId
 * @param {string} dateString - YYYY-MM-DD
 */
exports.getPeakHours = async (userId, dateString) => {
  const start = new Date(dateString);
  const end = new Date(dateString);
  end.setDate(end.getDate() + 1);

  const logs = await prisma.activityLog.findMany({
    where: {
      userId,
      timestamp: { gte: start, lt: end },
      processed: true
    }
  });

  // Initialize 24 hour slots
  const hours = [];
  for (let h = 0; h < 24; h++) {
    hours.push({
      hour: h,
      total_seconds: 0,
      productive: 0,
      distracting: 0,
      learning: 0,
      neutral: 0
    });
  }

  // Group logs by hour
  logs.forEach(log => {
    const hour = new Date(log.timestamp).getHours();
    const cat = log.category || 'neutral';

    hours[hour].total_seconds += log.durationSeconds;

    if (hours[hour][cat] !== undefined) {
      hours[hour][cat] += log.durationSeconds;
    } else {
      hours[hour].neutral += log.durationSeconds;
    }
  });

  // Find peak productive hour
  let peakHour = null;
  let maxProductive = 0;

  hours.forEach(slot => {
    if (slot.productive > maxProductive) {
      maxProductive = slot.productive;
      peakHour = slot.hour;
    }
  });

  // Find most active hour (any category)
  let mostActiveHour = null;
  let maxTotal = 0;

  hours.forEach(slot => {
    if (slot.total_seconds > maxTotal) {
      maxTotal = slot.total_seconds;
      mostActiveHour = slot.hour;
    }
  });

  return {
    date: dateString,
    peak_productive_hour: peakHour,
    most_active_hour: mostActiveHour,
    hourly_breakdown: hours.filter(h => h.total_seconds > 0)
  };
};
