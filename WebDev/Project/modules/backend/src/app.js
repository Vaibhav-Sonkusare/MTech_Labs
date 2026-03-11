require('dotenv').config();
const express = require('express');
const cors = require('cors');
const helmet = require('helmet');

const authRoutes = require('./routes/authRoutes');
const activityRoutes = require('./routes/activityRoutes');
const summaryRoutes = require('./routes/summaryRoutes');

const app = express();

app.use(cors());
app.use(helmet());
app.use(express.json());

app.use('/api/auth', authRoutes);
app.use('/api/activity', activityRoutes);
app.use('/api/summary', summaryRoutes);

app.get('/', (req, res) => {
  res.json({ message: 'Smart Digital Wellbeing Backend Running' });
});

module.exports = app;
