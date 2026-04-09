// Sync auth token from dashboard to extension
window.addEventListener("message", (event) => {
    if (event.data && event.data.type === "WELLBEING_SYNCAuth") {
        browser.runtime.sendMessage(event.data);
    }
});

// Check localStorage on dashboard load to keep everything in sync
if (window.location.host.includes("localhost:3000")) {
    const token = localStorage.getItem("access_token");
    const email = localStorage.getItem("user_email");
    if (token) {
        browser.runtime.sendMessage({ 
            type: "WELLBEING_SYNCAuth", 
            token: token, 
            email: email 
        });
    } else {
        browser.runtime.sendMessage({ 
            type: "WELLBEING_SYNCAuth", 
            token: null 
        });
    }
}
