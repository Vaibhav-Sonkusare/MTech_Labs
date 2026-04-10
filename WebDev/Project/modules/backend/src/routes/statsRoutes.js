const express = require('express');
const router = express.Router();

const statsController = require('../controllers/statsController');
const authMiddleware = require('../middleware/authMiddleware');
const { getStreak } = require('../analytics/streakAnalytics');

router.get('/categories', authMiddleware, statsController.getCategoryStats);
router.get('/peak-hours', authMiddleware, statsController.getPeakHours);

router.get('/streak', authMiddleware, async (req, res) => {
  try {
    const streak = await getStreak(req.userId);
    res.json(streak);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Failed to compute streak' });
  }
});

module.exports = router;
