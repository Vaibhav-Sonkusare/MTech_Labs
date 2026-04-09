import { AuthManager } from "./authManager.js";
import { API_BASE } from "./config.js";

/**
 * Detect the browser name for device registration.
 */
function getBrowserName() {
  if (typeof chrome !== 'undefined' && chrome.runtime && chrome.runtime.id) {
    if (navigator.userAgent.includes('Firefox')) return 'Firefox';
    return 'Chrome';
  }
  if (typeof browser !== 'undefined') return 'Firefox';
  return 'Browser';
}

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
                device_name: `${getBrowserName()} Extension`,
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
