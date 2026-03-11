const prisma = require('../config/prisma');
const aiService = require('../services/aiService');

exports.logActivity = async (req, res) => {
  try {
    const { domain, title, duration_seconds, timestamp } = req.body;

    // Basic validation
    if (!domain || !duration_seconds || !timestamp) {
      return res.status(400).json({ error: 'Missing required fields' });
    }

    // Insert raw log first
    const activity = await prisma.activityLog.create({
      data: {
        userId: req.userId,
        domain,
        title: title || '',
        durationSeconds: duration_seconds,
        timestamp: new Date(timestamp),
        processed: false
      }
    });

    // Call AI classification (mock)
    const { category, confidence } =
      await aiService.classifyActivity(domain, title);

    // Update same log
    await prisma.activityLog.update({
      where: { id: activity.id },
      data: {
        category,
        confidence,
        processed: true
      }
    });

    res.status(201).json({
      message: 'Activity logged successfully',
      category,
      confidence
    });

  } catch (error) {
    console.error(error);
    res.status(500).json({ error: 'Internal server error' });
  }
};
