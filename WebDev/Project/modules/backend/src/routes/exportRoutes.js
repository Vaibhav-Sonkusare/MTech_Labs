const prisma = require('../config/prisma');
const jwt = require('jsonwebtoken');
const router = require('express').Router();

/**
 * Auth middleware that supports both header and query param tokens.
 * Needed for CSV download (opens in new tab, can't set headers).
 */
function exportAuth(req, res, next) {
  const token = req.query.token ||
    (req.headers.authorization && req.headers.authorization.replace('Bearer ', ''));

  if (!token) {
    return res.status(401).json({ error: 'Authentication required' });
  }

  try {
    const decoded = jwt.verify(token, process.env.JWT_SECRET);
    req.userId = decoded.userId;
    next();
  } catch {
    return res.status(401).json({ error: 'Invalid token' });
  }
}

/**
 * GET /api/export/csv?start=YYYY-MM-DD&end=YYYY-MM-DD&token=JWT
 * Export activity logs as CSV.
 */
router.get('/csv', exportAuth, async (req, res) => {
  try {
    const { start, end } = req.query;
    const startDate = start ? new Date(start + 'T00:00:00') : new Date(0);
    const endDate = end ? new Date(end + 'T23:59:59') : new Date();

    const logs = await prisma.activityLog.findMany({
      where: {
        userId: req.userId,
        timestamp: { gte: startDate, lte: endDate },
      },
      include: { device: { select: { deviceName: true } } },
      orderBy: { timestamp: 'desc' },
    });

    // CSV header
    const header = 'Date,Time,Domain,Title,Category,Duration (min),Device\n';

    const rows = logs.map(log => {
      const dt = new Date(log.timestamp);
      const date = dt.toISOString().split('T')[0];
      const time = dt.toTimeString().split(' ')[0];
      const title = `"${(log.title || '').replace(/"/g, '""')}"`;
      const duration = (log.durationSeconds / 60).toFixed(1);
      const device = log.device?.deviceName || 'Unknown';
      return `${date},${time},${log.domain},${title},${log.category || 'neutral'},${duration},${device}`;
    }).join('\n');

    res.setHeader('Content-Type', 'text/csv');
    res.setHeader('Content-Disposition', `attachment; filename=wellbeing_${start || 'all'}_to_${end || 'now'}.csv`);
    res.send(header + rows);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Export failed' });
  }
});

module.exports = router;
