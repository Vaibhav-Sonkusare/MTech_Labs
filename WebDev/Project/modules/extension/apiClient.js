import { AuthManager } from "./authManager.js";
import { API_BASE } from "./config.js";

export async function sendActivity(activity) {

    let token = await AuthManager.getAccessToken();

    let res = await fetch(`${API_BASE}/api/activity`, {
        method: "POST",
        headers: {
            "Content-Type": "application/json",
            "Authorization": `Bearer ${token}`
        },
        body: JSON.stringify(activity)
    });

    if (res.status === 401) {

        token = await AuthManager.refreshAccessToken();

        res = await fetch(`${API_BASE}/api/activity`, {
            method: "POST",
            headers: {
                "Content-Type": "application/json",
                "Authorization": `Bearer ${token}`
            },
            body: JSON.stringify(activity)
        });

    }

    return res.ok;

}
