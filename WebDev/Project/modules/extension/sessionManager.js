import { extractDomain, durationSeconds, nowISO } from "./utils.js";
import { sendActivity } from "./apiClient.js";
import { DeviceManager } from "./deviceManager.js";

const MIN_SESSION_SECONDS = 5;
const MAX_SESSION_SECONDS = 3600;

export class SessionManager {

    constructor() {
        this.currentSession = null;
        this.lastUrl = null;
    }

    async startSession(tab) {

        if (!tab || !tab.url) return;

        if (this.currentSession && this.currentSession.url === tab.url) {
            return;
        }

        const domain = extractDomain(tab.url);
        if (!domain) return;

        this.currentSession = {
            tabId: tab.id,
            url: tab.url,
            domain: domain,
            title: tab.title || "",
            startTime: Date.now()
        };

        this.lastUrl = tab.url;

        console.log("Session started", this.currentSession);
    }

    async endSession() {

        if (!this.currentSession) return;

        const duration = durationSeconds(this.currentSession.startTime);

        if (duration < MIN_SESSION_SECONDS) {
            this.currentSession = null;
            return;
        }

        if (duration > MAX_SESSION_SECONDS) {
            this.currentSession = null;
            return;
        }

        const deviceId = await DeviceManager.ensureDeviceRegistered();

        const activity = {
            device_id: deviceId,
            domain: this.currentSession.domain,
            title: this.currentSession.title,
            duration_seconds: duration,
            timestamp: nowISO()
        };

        const success = await sendActivity(activity);

        if (!success) {
            await this.queueActivity(activity);
        }

        console.log("Session ended", activity);

        this.currentSession = null;

    }

    async queueActivity(activity) {

        const data = await browser.storage.local.get("activity_queue");

        const queue = data.activity_queue || [];

        queue.push(activity);

        await browser.storage.local.set({
            activity_queue: queue
        });

    }

}

export async function retryQueuedActivities() {

    const data = await browser.storage.local.get("activity_queue");

    const queue = data.activity_queue || [];

    if (queue.length === 0) return;

    const remaining = [];

    for (const activity of queue) {

        const success = await sendActivity(activity);

        if (!success) {
            remaining.push(activity);
        }

    }

    await browser.storage.local.set({
        activity_queue: remaining
    });

}
