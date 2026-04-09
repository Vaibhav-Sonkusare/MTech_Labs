require('dotenv').config();
const express = require('express');
const cors = require('cors');
const helmet = require('helmet');
const path = require('path');

const authRoutes = require('./routes/authRoutes');
const deviceRoutes = require('./routes/deviceRoutes');
const activityRoutes = require('./routes/activityRoutes');
const summaryRoutes = require('./routes/summaryRoutes');
const statsRoutes = require('./routes/statsRoutes');
const exportRoutes = require('./routes/exportRoutes');
const categoryRoutes = require('./routes/categoryRoutes');

const app = express();

app.use(cors());
app.use(helmet({ contentSecurityPolicy: false })); // Allow inline styles for dashboard
app.use(express.json());

// ─── API Routes ───
app.use('/api/auth', authRoutes);
app.use('/api/devices', deviceRoutes);
app.use('/api/activity', activityRoutes);
app.use('/api/summary', summaryRoutes);
app.use('/api/stats', statsRoutes);
app.use('/api/export', exportRoutes);
app.use('/api/categories', categoryRoutes);

// ─── Serve Dashboard (static build) ───
const dashboardPath = path.resolve(__dirname, '../../dashboard/dist');
app.use(express.static(dashboardPath));

// SPA catch-all: any non-API route → dashboard's index.html
app.get('/{*splat}', (req, res) => {
  if (req.path.startsWith('/api/')) {
    return res.status(404).json({ error: 'API route not found' });
  }
  res.sendFile(path.join(dashboardPath, 'index.html'));
});

module.exports = app;
