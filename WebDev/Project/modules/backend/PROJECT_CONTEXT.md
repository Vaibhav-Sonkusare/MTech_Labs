# Smart Digital Wellbeing & Productivity Analytics Platform
## Backend – Project Context

---

## 1. System Overview

This backend is part of a larger platform designed to track browser activity, classify it using AI, compute productivity analytics, and serve structured insights to a frontend dashboard.

The backend is implemented as a REST API using:

- Node.js
- Express.js
- Prisma ORM
- PostgreSQL
- JWT Authentication
- Bcrypt for password hashing

The system follows a modular, layered architecture.

---

## 2. Backend Responsibilities

The backend:

- Authenticates users
- Receives browser activity logs from extension
- Stores raw activity data
- Calls AI microservice for classification
- Stores classification results
- Computes productivity analytics
- Serves aggregated dashboard data

The backend does NOT:

- Capture browser data directly
- Train or host ML models
- Render frontend UI
- Use WebSockets (MVP is REST-based)

---

## 3. High-Level Architecture

Request Flow:

Browser Extension  
→ REST API (Express)  
→ Auth Middleware  
→ Controller  
→ Prisma (PostgreSQL)  
→ AI Service Layer  
→ Analytics Layer  
→ JSON Response  

The AI classification is abstracted inside a service module.

---

## 4. Tech Stack

Runtime: Node.js  
Framework: Express.js  
Database: PostgreSQL  
ORM: Prisma  
Authentication: JWT  
Password Hashing: bcrypt  
HTTP Client (AI calls): axios  
Environment Management: dotenv  
Security Middleware: helmet, cors  

---

## 5. Folder Structure
backend/
│
├── prisma/
│ └── schema.prisma
│
├── src/
│ ├── config/
│ │ └── prisma.js
│ │
│ ├── controllers/
│ │ ├── authController.js
│ │ ├── activityController.js
│ │ ├── summaryController.js
│ │
│ ├── routes/
│ │ ├── authRoutes.js
│ │ ├── activityRoutes.js
│ │ ├── summaryRoutes.js
│ │
│ ├── services/
│ │ └── aiService.js
│ │
│ ├── analytics/
│ │ └── dailyAnalytics.js
│ │
│ ├── middleware/
│ │ └── authMiddleware.js
│ │
│ └── app.js
│
├── server.js
└── .env

Architecture Rule:
- Controllers handle request/response only.
- Services handle business logic and external calls.
- Analytics layer handles aggregation logic.
- Middleware handles authentication and cross-cutting concerns.

---

## 6. Database Schema (Prisma)

### User

- id (UUID, primary key)
- email (unique)
- passwordHash
- createdAt

### ActivityLog

- id (UUID)
- userId (FK → User)
- domain
- title
- durationSeconds
- category (nullable until classified)
- confidence (nullable)
- processed (boolean)
- timestamp
- indexed on userId and timestamp

### DailySummary

- id (UUID)
- userId (FK)
- date
- productiveTime
- distractingTime
- neutralTime
- learningTime
- score
- unique constraint on (userId, date)

Currently daily summaries are computed on-demand (not persisted).

---

## 7. Implemented API Endpoints

### Authentication

POST   /api/auth/register
POST   /api/auth/login
GET    /api/auth/me

Authentication is JWT-based.
Protected routes require:

Authorization: Bearer <token>

---

### Activity Logging

POST /api/activity

Input:

{
  "domain": "github.com",
  "title": "Prisma Docs",
  "duration_seconds": 300,
  "timestamp": "2026-03-03T14:00:00Z"
}

Flow:
1. Validate JWT
2. Insert raw log
3. Call AI service
4. Update log with category + confidence
5. Return success

---

### Daily Summary

GET /api/summary/daily?date=YYYY-MM-DD

Response example:

{
  "date": "2026-03-03",
  "productive_time": 600,
  "distracting_time": 300,
  "neutral_time": 120,
  "learning_time": 0,
  "total_time": 1020,
  "score": 0.59
}

Score formula (current version):

score = productive_time / total_time

---

## 8. AI Integration

AI classification is abstracted in:

src/services/aiService.js

Currently:
- Mock implementation (rule-based)

Future:
- Replace with axios call to FastAPI service
- Endpoint: POST http://ai-service/classify
- Input: { domain, title }
- Output: { category, confidence }

---

## 9. Internal Design Principles

- Clean separation of concerns
- Stateless authentication
- Modular structure
- Database indexing for analytics performance
- Future-ready for async processing (processed flag)
- RESTful API design
- No frontend coupling

---

## 10. Current MVP Status

Working:

- User registration & login
- JWT authentication
- Activity logging
- Mock AI classification
- Daily summary analytics
- Prisma + PostgreSQL integration

Not yet implemented:

- Weekly summary
- Category statistics endpoint
- Peak hours analytics
- Weighted productivity scoring
- Real AI microservice integration
- Precomputed daily summaries
- Input validation library (Zod/Joi)
- Rate limiting
- Logging system
- Automated tests

---

## 11. One-Sentence Backend Summary

A modular Node.js + Express REST API that authenticates users, receives browser activity logs, classifies them via an AI service layer, stores structured data in PostgreSQL using Prisma ORM, computes productivity analytics, and serves aggregated dashboard-ready JSON responses.

---
