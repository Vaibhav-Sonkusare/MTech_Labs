const prisma = require('../config/prisma');

exports.registerDevice = async (req, res) => {

    try {

        const { device_name, device_type } = req.body;

        if (!device_name || !device_type) {
            return res.status(400).json({
                error: "device_name and device_type required"
            });
        }

        const device = await prisma.device.create({
            data: {
                userId: req.userId,
                deviceName: device_name,
                deviceType: device_type
            }
        });

        res.json({
            device_id: device.id
        });

    } catch (err) {
        console.error(err);
        res.status(500).json({ error: "Internal server error" });
    }

};