const prisma = require('../config/prisma');
const auth = require('../middleware/authMiddleware');
const router = require('express').Router();
const classifier = require('../services/classifier');

/**
 * GET /api/categories/custom — list user's custom overrides
 */
router.get('/custom', auth, async (req, res) => {
  try {
    const overrides = await prisma.customCategory.findMany({
      where: { userId: req.userId },
      select: { domain: true, category: true },
      orderBy: { domain: 'asc' },
    });
    res.json({ overrides });
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Failed to fetch custom categories' });
  }
});

/**
 * PUT /api/categories/custom — set a custom category override
 * Body: { domain, category }
 */
router.put('/custom', auth, async (req, res) => {
  try {
    const { domain, category } = req.body;
    if (!domain || !category) {
      return res.status(400).json({ error: 'domain and category required' });
    }

    const validCategories = ['productive', 'learning', 'neutral', 'distracting'];
    if (!validCategories.includes(category)) {
      return res.status(400).json({ error: 'Invalid category' });
    }

    const override = await prisma.customCategory.upsert({
      where: { userId_domain: { userId: req.userId, domain } },
      update: { category },
      create: { userId: req.userId, domain, category },
    });

    // Retroactively update all existing activity logs for this domain
    await prisma.activityLog.updateMany({
      where: { userId: req.userId, domain },
      data: { category }
    });

    // Demolish all cached Daily Summaries so they explicitly recalculate next fetch
    await prisma.dailySummary.deleteMany({
      where: { userId: req.userId }
    });

    res.json({ domain: override.domain, category: override.category });
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Failed to save custom category' });
  }
});

/**
 * DELETE /api/categories/custom/:domain — remove override (revert to auto)
 */
router.delete('/custom/:domain', auth, async (req, res) => {
  try {
    const domain = decodeURIComponent(req.params.domain);
    await prisma.customCategory.deleteMany({
      where: { userId: req.userId, domain },
    });

    // Replay classification engine against historical titles
    const logs = await prisma.activityLog.findMany({
      where: { userId: req.userId, domain },
      select: { id: true, title: true }
    });

    const updates = logs.map(log => {
      const autoClass = classifier.classify(domain, log.title);
      return prisma.activityLog.update({
        where: { id: log.id },
        data: { category: autoClass ? autoClass.category : 'neutral' }
      });
    });
    await Promise.all(updates);

    // Demolish all cached Daily Summaries
    await prisma.dailySummary.deleteMany({
      where: { userId: req.userId }
    });

    res.json({ deleted: true });
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Failed to delete custom category' });
  }
});

module.exports = router;
