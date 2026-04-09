const express = require('express');
const router = express.Router();

const summaryController = require('../controllers/summaryController');
const authMiddleware = require('../middleware/authMiddleware');

router.get('/daily', authMiddleware, summaryController.getDailySummary);
router.get('/weekly', authMiddleware, summaryController.getWeeklySummary);

module.exports = router;
