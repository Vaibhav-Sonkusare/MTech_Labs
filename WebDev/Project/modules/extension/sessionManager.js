import { extractDomain, durationSeconds, nowISO } from "./utils.js";
import { sendActivity } from "./apiClient.js";
import { DeviceManager } from "./deviceManager.js";

const MIN_SESSION_SECONDS = 5;

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
            startTime: Date.now(),
            lastHeartbeat: Date.now()
        };

        this.lastUrl = tab.url;

    }

    updateHeartbeat() {
        if (this.currentSession) {
            this.currentSession.lastHeartbeat = Date.now();
        }
    }

    async endSession() {

        if (!this.currentSession) return;

        let endTime = Date.now();

        // If it has been more than 60 seconds since the last heartbeat, the computer was asleep/suspended.
        // Cap the session end exactly to the last known heartbeat.
        if (endTime - this.currentSession.lastHeartbeat > 60000) {
            endTime = this.currentSession.lastHeartbeat;
        }

        const duration = Math.round((endTime - this.currentSession.startTime) / 1000);

        if (duration < MIN_SESSION_SECONDS) {
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
