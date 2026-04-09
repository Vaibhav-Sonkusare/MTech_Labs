const categoryAnalytics = require('../analytics/categoryAnalytics');
const peakHoursAnalytics = require('../analytics/peakHoursAnalytics');

exports.getCategoryStats = async (req, res) => {
  try {
    const { date } = req.query;

    if (!date) {
      return res.status(400).json({ error: 'Date query parameter required' });
    }

    const stats =
      await categoryAnalytics.getCategoryDistribution(req.userId, date);

    res.json(stats);

  } catch (error) {
    console.error(error);
    res.status(500).json({ error: 'Internal server error' });
  }
};

exports.getPeakHours = async (req, res) => {
  try {
    const { date } = req.query;

    if (!date) {
      return res.status(400).json({ error: 'Date query parameter required' });
    }

    const stats =
      await peakHoursAnalytics.getPeakHours(req.userId, date);

    res.json(stats);

  } catch (error) {
    console.error(error);
    res.status(500).json({ error: 'Internal server error' });
  }
};
