import { AuthManager } from "./authManager.js";

const API_URL = "http://localhost:3000";

export async function sendActivity(activity) {

    let token = await AuthManager.getAccessToken();

    let res = await fetch(`${API_URL}/api/activity`, {
        method: "POST",
        headers: {
            "Content-Type": "application/json",
            "Authorization": `Bearer ${token}`
        },
        body: JSON.stringify(activity)
    });

    if (res.status === 401) {

        token = await AuthManager.refreshAccessToken();

        res = await fetch(`${API_URL}/api/activity`, {
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
