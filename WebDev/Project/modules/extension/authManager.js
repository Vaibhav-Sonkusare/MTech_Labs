const API_BASE = "http://localhost:3000";

export class AuthManager {


    static async getAccessToken() {

        const data = await browser.storage.local.get([
            "access_token",
            "refresh_token"
        ]);

        if (!data.access_token) return null;

        return data.access_token;

    }

    static async refreshAccessToken() {

        const data = await browser.storage.local.get("refresh_token");

        if (!data.refresh_token) return null;

        const res = await fetch(`${API_BASE}/api/auth/refresh`, {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify({
                refresh_token: data.refresh_token
            })
        });

        if (!res.ok) return null;

        const json = await res.json();

        await browser.storage.local.set({
            access_token: json.access_token
        });

        return json.access_token;

    }


}
