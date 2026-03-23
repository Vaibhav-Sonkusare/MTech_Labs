🧠 Smart Digital Wellbeing & Productivity Analytics Platform
Backend Development Summary

The backend is a Node.js + Express REST API that authenticates users, registers devices, receives browser activity logs, classifies them using an AI service, stores them in PostgreSQL using Prisma ORM, and computes productivity analytics for a dashboard.

The system is designed to support multiple devices per user, secure authentication, and scalable analytics processing.

1️⃣ Technology Stack
Backend Runtime

Node.js

Web Framework

Express.js

Database

PostgreSQL

ORM

Prisma

Authentication

JWT

Access Token (15 minutes)

Refresh Token (30 days)

Security

bcrypt → password hashing

helmet → HTTP security headers

cors → cross-origin protection

AI Integration

External FastAPI classification service (currently mocked)

2️⃣ Backend Architecture

The backend follows a layered modular architecture.

Routes
  ↓
Controllers
  ↓
Services
  ↓
Database (Prisma)
  ↓
Analytics Layer

Responsibilities are separated so that:

Layer	Responsibility
Routes	Define API endpoints
Controllers	Handle requests/responses
Services	External services (AI)
Middleware	Authentication
Analytics	Data aggregation logic
Prisma	Database access
3️⃣ Folder Structure
backend/
│
├── prisma/
│   └── schema.prisma
│
├── src/
│   ├── config/
│   │     └── prisma.js
│   │
│   ├── controllers/
│   │     ├── authController.js
│   │     ├── activityController.js
│   │     ├── deviceController.js
│   │     └── summaryController.js
│   │
│   ├── routes/
│   │     ├── authRoutes.js
│   │     ├── activityRoutes.js
│   │     ├── deviceRoutes.js
│   │     └── summaryRoutes.js
│   │
│   ├── services/
│   │     └── aiService.js
│   │
│   ├── analytics/
│   │     └── dailyAnalytics.js
│   │
│   ├── middleware/
│   │     └── authMiddleware.js
│   │
│   └── app.js
│
├── server.js
└── .env

This modular structure prevents tightly coupled code.

4️⃣ Database Design (Prisma)

The database contains three core models.

User

Represents a registered platform user.

Fields:

id
email
passwordHash
createdAt

Relationships:

User
 ├── Devices
 └── ActivityLogs
Device

Represents a browser extension instance.

Example devices:

Firefox extension

Chrome extension

Edge extension

Fields:

id
userId
deviceName
deviceType
createdAt

Relationship:

Device
 └── ActivityLogs
ActivityLog

Stores browsing activity.

Fields:

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

Indexes:

userId
deviceId
timestamp
5️⃣ Authentication System

Authentication uses JWT tokens with refresh flow.

Access Token

Valid for 15 minutes

Sent in headers

Authorization: Bearer <token>
Refresh Token

Valid for 30 days

Used to generate new access tokens

6️⃣ Authentication Endpoints
Register
POST /api/auth/register

Body

{
  "email": "user@email.com",
  "password": "password"
}

Behavior

password hashed using bcrypt

user stored in database

access + refresh tokens returned

Login
POST /api/auth/login

Response

{
  "access_token": "...",
  "refresh_token": "..."
}
Refresh Token
POST /api/auth/refresh

Input

{
  "refresh_token": "..."
}

Output

{
  "access_token": "..."
}
7️⃣ Device Registration System

Browser extensions must register as devices.

Endpoint:

POST /api/devices/register

Headers

Authorization: Bearer ACCESS_TOKEN

Body

{
  "device_name": "Firefox Browser",
  "device_type": "browser_extension"
}

Backend actions:

Verify JWT

Extract userId

Create device record

Return device_id

Response

{
  "device_id": "uuid"
}
8️⃣ Activity Logging System

This is the core feature of the platform.

Endpoint:

POST /api/activity

Headers

Authorization: Bearer ACCESS_TOKEN

Body

{
  "device_id": "uuid",
  "domain": "github.com",
  "title": "Prisma Docs",
  "duration_seconds": 300,
  "timestamp": "2026-03-03T14:00:00Z"
}
Activity Processing Flow
Extension
   ↓
POST /api/activity
   ↓
JWT verification
   ↓
Verify device belongs to user
   ↓
Insert activity log
   ↓
Call AI classification service
   ↓
Update category + confidence
   ↓
Return success response
9️⃣ AI Classification Layer

Located in:

src/services/aiService.js

Current implementation:

Mock classification logic

Future implementation:

POST http://ai-service/classify

Input

{
  domain,
  title
}

Output

{
  category,
  confidence
}
🔟 Daily Summary Analytics

Endpoint:

GET /api/summary/daily?date=YYYY-MM-DD

Purpose:

Compute productivity statistics.

Steps:

Fetch activity logs for that day

Group by category

Compute time totals

Calculate productivity score

Example Response
{
  "date": "2026-03-03",
  "productive_time": 600,
  "distracting_time": 300,
  "neutral_time": 120,
  "learning_time": 0,
  "total_time": 1020,
  "score": 0.59
}

Productivity score formula:

score = productive_time / total_time
1️⃣1️⃣ Browser Extension Workflow

The extension communicates with the backend using this flow.

Step 1 — Login
POST /api/auth/login

Store:

access_token
refresh_token
Step 2 — Register Device
POST /api/devices/register

Store:

device_id
Step 3 — Send Activity Logs
POST /api/activity

Payload includes:

device_id
domain
title
duration_seconds
timestamp
1️⃣2️⃣ Security Measures

The backend enforces:

JWT verification on protected routes

password hashing using bcrypt

device ownership validation

short-lived access tokens

refresh token rotation

These prevent:

unauthorized activity submissions

device spoofing

session hijacking

1️⃣3️⃣ Current System Capabilities

The backend now supports:

✔ User registration and login
✔ Access/refresh token authentication
✔ Multi-device user accounts
✔ Secure device registration
✔ Per-device activity tracking
✔ AI-based activity classification
✔ Daily productivity analytics
✔ Modular backend architecture

1️⃣4️⃣ Current System Architecture
User
 ├── Device (Firefox Extension)
 │       └── ActivityLogs
 │
 ├── Device (Chrome Extension)
 │       └── ActivityLogs
 │
 └── Analytics

This architecture allows future expansion to:

mobile apps

desktop monitoring agents

team productivity dashboards

1️⃣5️⃣ Features Not Yet Implemented

Planned improvements include:

weekly summary analytics

category statistics endpoint

peak productivity hours

real AI microservice integration

background job processing

rate limiting

request validation

logging system

automated tests

deployment pipeline

✅ Final One-Sentence Summary

The backend is a modular Node.js + Express REST API that authenticates users, registers browser devices, securely receives browsing activity logs, classifies them via an AI service, stores structured data in PostgreSQL using Prisma ORM, and computes productivity analytics for a dashboard.