const API_BASE = "http://localhost:3000";

document.getElementById("loginBtn").addEventListener("click", async () => {

    const email = document.getElementById("email").value;
    const password = document.getElementById("password").value;
    const status = document.getElementById("status");

    try {

        const res = await fetch(`${API_BASE}/api/auth/login`, {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify({ email, password })
        });

        if (!res.ok) {
            throw new Error("Login failed");
        }

        const data = await res.json();

        await browser.storage.local.set({
            access_token: data.access_token,
            refresh_token: data.refresh_token
        });

        status.textContent = "Login successful.";

    } catch (err) {

        status.textContent = "Login failed.";

    }

});
