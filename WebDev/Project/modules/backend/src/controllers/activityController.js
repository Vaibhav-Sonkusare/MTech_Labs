const prisma = require('../config/prisma');
const aiService = require('../services/aiService');

exports.logActivity = async (req, res) => {

  try {

    const {
      device_id,
      domain,
      title,
      duration_seconds,
      timestamp
    } = req.body;

    if (!device_id || !domain || !duration_seconds || !timestamp) {
      return res.status(400).json({
        error: "Missing required fields"
      });
    }

    // 1️⃣ Verify device belongs to the user
    const device = await prisma.device.findFirst({
      where: {
        id: device_id,
        userId: req.userId
      }
    });

    if (!device) {
      return res.status(403).json({
        error: "Invalid device"
      });
    }

    // 2️⃣ Insert raw activity log
    const activity = await prisma.activityLog.create({
      data: {
        userId: req.userId,
        deviceId: device_id,
        domain,
        title,
        durationSeconds: duration_seconds,
        timestamp: new Date(timestamp),
        processed: false
      }
    });

    // 3️⃣ Call AI classification service (with user overrides)
    const { category, confidence } =
      await aiService.classifyWithOverrides(domain, title, req.userId, prisma);

    // 4️⃣ Update log with classification
    await prisma.activityLog.update({
      where: { id: activity.id },
      data: {
        category,
        confidence,
        processed: true
      }
    });

    res.status(201).json({
      message: "Activity logged",
      category,
      confidence
    });

  } catch (err) {

    console.error(err);
    res.status(500).json({ error: "Internal server error" });

  }

};