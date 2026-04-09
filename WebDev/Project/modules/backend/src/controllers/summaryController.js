const dailyAnalytics = require('../analytics/dailyAnalytics');
const weeklyAnalytics = require('../analytics/weeklyAnalytics');

exports.getDailySummary = async (req, res) => {
  try {
    const { date } = req.query;

    if (!date) {
      return res.status(400).json({ error: 'Date query parameter required' });
    }

    const summary =
      await dailyAnalytics.calculateDailySummary(req.userId, date);

    res.json(summary);

  } catch (error) {
    console.error(error);
    res.status(500).json({ error: 'Internal server error' });
  }
};

exports.getWeeklySummary = async (req, res) => {
  try {
    const { date } = req.query;

    if (!date) {
      return res.status(400).json({ error: 'Date query parameter required' });
    }

    const summary =
      await weeklyAnalytics.calculateWeeklySummary(req.userId, date);

    res.json(summary);

  } catch (error) {
    console.error(error);
    res.status(500).json({ error: 'Internal server error' });
  }
};
