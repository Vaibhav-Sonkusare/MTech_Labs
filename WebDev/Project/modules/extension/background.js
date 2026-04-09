import { SessionManager, retryQueuedActivities } from "./sessionManager.js";
import { API_BASE } from "./config.js";

const sessionManager = new SessionManager();

// ─── Tab Event Listeners (activity tracking) ───

browser.tabs.onActivated.addListener(async (activeInfo) => {
    await sessionManager.endSession();
    const tab = await browser.tabs.get(activeInfo.tabId);
    await sessionManager.startSession(tab);
    broadcastTabs();
});

browser.tabs.onUpdated.addListener(async (tabId, changeInfo, tab) => {
    if (changeInfo.url) {
        await sessionManager.endSession();
        await sessionManager.startSession(tab);
    }
    if (changeInfo.title || changeInfo.url || changeInfo.status === 'complete') {
        broadcastTabs();
    }
});

browser.tabs.onCreated.addListener(() => {
    broadcastTabs();
});

browser.tabs.onRemoved.addListener(async (tabId) => {
    if (sessionManager.currentSession &&
        sessionManager.currentSession.tabId === tabId) {
        await sessionManager.endSession();
    }
    broadcastTabs();
});

browser.windows.onFocusChanged.addListener(async (windowId) => {
    if (windowId === browser.windows.WINDOW_ID_NONE) {
        if (sessionManager.currentSession) {
            try {
                const tab = await browser.tabs.get(sessionManager.currentSession.tabId);
                const win = await browser.windows.get(tab.windowId);
                if (win.state === "fullscreen" || tab.audible) {
                    return; // Keep recording, user is watching full screen or listening to media
                }
            } catch (e) {
                console.error("Error fetching window state", e);
            }
        }
        await sessionManager.endSession();
    } else {
        await sessionManager.endSession();
        try {
            const tabs = await browser.tabs.query({ active: true, windowId: windowId });
            if (tabs.length > 0) {
                await sessionManager.startSession(tabs[0]);
            }
        } catch (e) {
            console.error("Error switching window session", e);
        }
    }
    broadcastTabs();
});

browser.idle.onStateChanged.addListener(async (state) => {
    if (state === "idle" || state === "locked") {
        if (sessionManager.currentSession) {
            try {
                const tab = await browser.tabs.get(sessionManager.currentSession.tabId);
                const win = await browser.windows.get(tab.windowId);
                if (win.state === "fullscreen" || tab.audible) {
                    return; // Ignore idle state, user is watching media
                }
            } catch (e) {
                console.error("Error fetching window state", e);
            }
        }
        await sessionManager.endSession();
    } else if (state === "active") {
        if (!sessionManager.currentSession) {
           const tabs = await browser.tabs.query({ active: true, currentWindow: true });
           if (tabs.length > 0) {
               await sessionManager.startSession(tabs[0]);
           }
        }
    }
});

async function initializeTracking() {
    const tabs = await browser.tabs.query({
        active: true,
        currentWindow: true
    });
    if (tabs.length > 0) {
        await sessionManager.startSession(tabs[0]);
    }
}

initializeTracking();

setInterval(() => {
    retryQueuedActivities();
}, 30000);

setInterval(() => {
    sessionManager.updateHeartbeat();
}, 15000);

// ─── WebSocket Connection (live tabs to dashboard) ───

let ws = null;
let wsReconnectTimer = null;

function generateUUID() {
    return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
        var r = Math.random() * 16 | 0, v = c === 'x' ? r : (r & 0x3 | 0x8);
        return v.toString(16);
    });
}

async function connectWebSocket() {
    let data = await browser.storage.local.get(["access_token", "deviceId"]);
    if (!data.access_token) return;

    let deviceId = data.deviceId;
    if (!deviceId) {
        deviceId = generateUUID();
        await browser.storage.local.set({ deviceId });
    }

    const wsUrl = API_BASE.replace('http', 'ws');

    try {
        ws = new WebSocket(`${wsUrl}`);

        ws.onopen = () => {
            ws.send(JSON.stringify({
                type: "auth",
                token: data.access_token,
                clientType: "extension",
                deviceName: navigator.userAgent.includes("Firefox") ? "Firefox" : "Browser",
                deviceId: deviceId
            }));
            // Send initial tab snapshot
            broadcastTabs();
        };

        ws.onmessage = (event) => {
            const msg = JSON.parse(event.data);
            if (msg.type === 'auth_error') {
                ws.close();
            }
        };

        ws.onclose = () => {
            ws = null;
            // Reconnect after 5 seconds
            clearTimeout(wsReconnectTimer);
            wsReconnectTimer = setTimeout(connectWebSocket, 5000);
        };

        ws.onerror = () => {
            ws?.close();
        };
    } catch (e) {
        console.error("WebSocket connection error:", e);
        // Retry later
        clearTimeout(wsReconnectTimer);
        wsReconnectTimer = setTimeout(connectWebSocket, 5000);
    }
}

async function broadcastTabs() {
    if (!ws || ws.readyState !== WebSocket.OPEN) return;

    try {
        const allTabs = await browser.tabs.query({});
        const activeTabs = await browser.tabs.query({ active: true, currentWindow: true });
        const activeTabId = activeTabs.length > 0 ? activeTabs[0].id : null;

        const tabs = allTabs.map(tab => ({
            id: tab.id,
            url: tab.url || "",
            title: tab.title || "",
            favIconUrl: tab.favIconUrl || "",
            active: tab.id === activeTabId,
            windowId: tab.windowId,
        }));

        ws.send(JSON.stringify({
            type: "tabs_update",
            tabs,
            activeTabId
        }));
    } catch {
        // Tab query can fail in some edge cases
    }
}

// Connect on startup
connectWebSocket();

// Reconnect when token changes (user logs in)
browser.storage.onChanged.addListener((changes) => {
    if (changes.access_token) {
        if (ws) ws.close();
        connectWebSocket();
    }
});

// Listen for Auth Sync from dashboard content script
browser.runtime.onMessage.addListener((message) => {
    if (message.type === "WELLBEING_SYNCAuth") {
        if (message.token) {
            browser.storage.local.set({ 
                access_token: message.token, 
                user_email: message.email || "" 
            });
        } else {
            browser.storage.local.remove(["access_token", "refresh_token", "user_email"]);
        }
    }
});
