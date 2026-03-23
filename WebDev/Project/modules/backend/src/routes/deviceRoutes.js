const express = require('express');
const router = express.Router();

const deviceController = require('../controllers/deviceController');
const authMiddleware = require('../middleware/authMiddleware');

router.post('/register', authMiddleware, deviceController.registerDevice);

module.exports = router;