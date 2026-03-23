Smart Digital Wellbeing & Productivity Analytics Platform
Development Summary (Current State)

This project is a full-stack digital wellbeing analytics system that tracks browsing activity, classifies websites using AI, and produces productivity insights.

The system consists of the following components:

Browser Extension
      ↓
Node.js Backend API
      ↓
PostgreSQL Database
      ↓
AI Classification Service
      ↓
Analytics Engine
      ↓
Dashboard Frontend (to be built)

The browser extension acts purely as a data collection agent, while all analytics and classification happen on the backend.

1. Backend Architecture

The backend is built using:

Node.js

Express

PostgreSQL

Prisma ORM

JWT authentication

AI classification service

The backend now supports multi-device authenticated activity tracking.

2. Database Schema
Users

Represents platform users.

User
-----
id (UUID)
email
password_hash
created_at
Devices

Each browser extension installation registers as a device.

Device
------
id (UUID)
userId (FK → User.id)
deviceName
deviceType
createdAt

Example:

Firefox Browser
browser_extension

A user can own multiple devices.

Activity Logs

Stores browsing sessions collected by the extension.

ActivityLog
-----------
id
userId
deviceId
domain
title
durationSeconds
category
confidence
timestamp
processed

Relationships:

User
 ├── Devices
 │      └── ActivityLogs
 │
 └── ActivityLogs

This enables per-device analytics and multi-device tracking.

3. Authentication System

The system now uses a secure JWT + refresh token architecture.

Access Token
Lifetime: 15 minutes

Used for authenticated API requests.

Refresh Token
Lifetime: 30 days

Used to obtain new access tokens without requiring user login.

Authentication Endpoints
POST /api/auth/register
POST /api/auth/login
POST /api/auth/refresh

Extension workflow:

login
→ store access_token + refresh_token
→ auto refresh when needed
4. Device Registration

After login, the extension registers itself.

Endpoint:

POST /api/devices/register

Headers:

Authorization: Bearer ACCESS_TOKEN

Body:

{
  "device_name": "Firefox Browser",
  "device_type": "browser_extension"
}

Response:

device_id

This ID is stored locally in the extension.

5. Activity Logging API

The extension sends browsing sessions to:

POST /api/activity

Payload:

{
  device_id,
  domain,
  title,
  duration_seconds,
  timestamp
}

Backend flow:

1. Verify JWT
2. Extract userId
3. Verify device ownership
4. Store activity log
5. Send data to AI classifier
6. Update category + confidence
6. AI Classification Layer

The backend integrates with an AI service located in:

src/services/aiService.js

Current implementation:

Mock classifier

Future implementation:

POST http://ai-service/classify

Input:

{ domain, title }

Output:

{ category, confidence }

Example categories:

Productive
Learning
Social Media
Entertainment
Shopping
7. Browser Extension Architecture

The extension is built using the WebExtensions API with Firefox as the primary target.

Project structure:

browser-extension/

manifest.json
background.js
sessionManager.js
authManager.js
deviceManager.js
apiClient.js
utils.js
popup.html
popup.js
8. Extension Authentication Flow
User installs extension
      ↓
Login through popup
      ↓
Store:
access_token
refresh_token
      ↓
Register device
      ↓
Store device_id
      ↓
Begin activity tracking

Tokens and device ID are stored in:

browser.storage.local

These persist until the extension is uninstalled.

9. Event-Driven Activity Tracking

The extension uses a professional event-driven session model.

Instead of a timer, it listens to browser events.

Events monitored:

tabs.onActivated
tabs.onUpdated
tabs.onRemoved
windows.onFocusChanged
idle.onStateChanged
10. Session Tracking Model

The extension maintains one active session.

Example session:

{
 tabId,
 domain,
 title,
 startTime
}

When events occur:

endSession()
→ compute duration
→ ignore <5 seconds
→ send activity log
startSession()
11. Noise Filtering

Sessions shorter than:

5 seconds

are discarded to avoid analytics noise.

12. Idle and Focus Handling

The extension detects when the user is not actively browsing.

Triggers session termination when:

user idle
screen locked
browser window loses focus
13. Single Page Application Handling

Modern sites update content without changing tabs.

The extension detects:

changeInfo.url

and splits sessions accordingly.

This ensures accurate tracking on:

YouTube
Twitter
Reddit
Gmail
14. Duplicate Session Protection

Some browser events fire repeatedly.

The extension prevents duplicates by checking:

currentSession.url === tab.url

before creating a new session.

15. Network Failure Protection

If the backend is temporarily unreachable:

activity logs → local queue

Stored in:

browser.storage.local

A background worker retries every:

30 seconds
16. Startup Recovery

When the extension starts:

browser.tabs.query({ active: true })

This ensures activity tracking begins immediately if a tab is already open.

17. Current System Capabilities

The platform now supports:

✔ user authentication
✔ refresh token flow
✔ multiple devices per user
✔ per-device activity tracking
✔ AI-based classification
✔ event-driven browser tracking
✔ idle detection
✔ network retry queue
✔ duplicate event suppression
✔ session-based analytics
18. Testing Status

The extension has been successfully tested.

Observed behavior:

✔ sessions start correctly
✔ sessions end correctly
✔ device registration works
✔ tokens stored properly
✔ activity logs sent to backend
✔ database records created

Example log:

youtube.com
duration: 78 seconds
19. System Architecture (Current)
User
 ├── Device (Firefox Extension)
 │       └── ActivityLogs
 │
 ├── Device (Chrome Extension - future)
 │       └── ActivityLogs
 │
 └── Analytics Dashboard (future)
20. Next Development Phase

The next major component is the frontend analytics dashboard.

Planned features:

Daily activity summary
Productive vs distracting time
Top visited domains
Timeline of activity
Category breakdown

The frontend will consume APIs such as:

GET /api/analytics/daily
GET /api/analytics/domains
GET /api/analytics/productivity
Final Status

The project currently has a fully operational backend and browser activity collector.

The system is now ready for development of the analytics dashboard frontend, which will visualize user productivity insights derived from collected browsing data.