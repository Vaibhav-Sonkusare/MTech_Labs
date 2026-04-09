const prisma = require('../config/prisma');
const auth = require('../middleware/authMiddleware');
const router = require('express').Router();

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
    res.json({ deleted: true });
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Failed to delete custom category' });
  }
});

module.exports = router;
