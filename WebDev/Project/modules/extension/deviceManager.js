import { AuthManager } from "./authManager.js";

const API_BASE = "http://localhost:3000";

export class DeviceManager {

    static async ensureDeviceRegistered() {

        const data = await browser.storage.local.get("device_id");

        if (data.device_id) return data.device_id;

        const token = await AuthManager.getAccessToken();

        const res = await fetch(`${API_BASE}/api/devices/register`, {
            method: "POST",
            headers: {
                "Content-Type": "application/json",
                "Authorization": `Bearer ${token}`
            },
            body: JSON.stringify({
                device_name: "Firefox Browser",
                device_type: "browser_extension"
            })
        });

        if (!res.ok) throw new Error("Device registration failed");

        const json = await res.json();

        await browser.storage.local.set({
            device_id: json.device_id
        });

        return json.device_id;

    }

}
