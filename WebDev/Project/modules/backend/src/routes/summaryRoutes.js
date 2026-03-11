const express = require('express');
const router = express.Router();

const summaryController = require('../controllers/summaryController');
const authMiddleware = require('../middleware/authMiddleware');

router.get('/daily', authMiddleware, summaryController.getDailySummary);

module.exports = router;
