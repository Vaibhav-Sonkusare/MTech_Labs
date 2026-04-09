const prisma = require('../config/prisma');

/**
 * Get category distribution for a given date.
 *
 * Returns percentage breakdown and top 5 domains per category.
 *
 * @param {string} userId
 * @param {string} dateString - YYYY-MM-DD
 */
exports.getCategoryDistribution = async (userId, dateString) => {
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

  // Group by category
  const categoryMap = {};

  logs.forEach(log => {
    const cat = log.category || 'neutral';

    if (!categoryMap[cat]) {
      categoryMap[cat] = { total_seconds: 0, domains: {} };
    }

    categoryMap[cat].total_seconds += log.durationSeconds;

    // Track per-domain time within each category
    if (!categoryMap[cat].domains[log.domain]) {
      categoryMap[cat].domains[log.domain] = 0;
    }
    categoryMap[cat].domains[log.domain] += log.durationSeconds;
  });

  // Calculate total time across all categories
  const totalTime = Object.values(categoryMap)
    .reduce((sum, c) => sum + c.total_seconds, 0);

  // Build response array
  const categories = Object.entries(categoryMap).map(([category, data]) => {
    // Top 5 domains for this category, sorted by time desc
    const topDomains = Object.entries(data.domains)
      .sort((a, b) => b[1] - a[1])
      .slice(0, 5)
      .map(([domain, seconds]) => ({ domain, seconds }));

    return {
      category,
      total_seconds: data.total_seconds,
      percentage: totalTime === 0
        ? 0
        : Number(((data.total_seconds / totalTime) * 100).toFixed(1)),
      top_domains: topDomains
    };
  });

  // Sort by total_seconds descending
  categories.sort((a, b) => b.total_seconds - a.total_seconds);

  return {
    date: dateString,
    total_time: totalTime,
    categories
  };
};
