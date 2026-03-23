import { SessionManager, retryQueuedActivities } from "./sessionManager.js";
const sessionManager = new SessionManager();

browser.tabs.onActivated.addListener(async (activeInfo) => {
    await sessionManager.endSession();

    const tab = await browser.tabs.get(activeInfo.tabId);

    await sessionManager.startSession(tab);
});

browser.tabs.onUpdated.addListener(async (tabId, changeInfo, tab) => {
    if (changeInfo.url) {

        await sessionManager.endSession();

        await sessionManager.startSession(tab);

    }
});

browser.tabs.onRemoved.addListener(async (tabId) => {
    if (sessionManager.currentSession &&
        sessionManager.currentSession.tabId === tabId) {

        await sessionManager.endSession();

    }
});

browser.windows.onFocusChanged.addListener(async (windowId) => {
    if (windowId === browser.windows.WINDOW_ID_NONE) {

        await sessionManager.endSession();

    }
});

browser.idle.onStateChanged.addListener(async (state) => {
    if (state === "idle" || state === "locked") {

        await sessionManager.endSession();

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
