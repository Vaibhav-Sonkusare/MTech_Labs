const request = require("supertest");
const app = require("../src/app");
const prisma = require("../src/config/prisma");

describe("Auth + Activity + Summary Integration Tests", () => {
  let token;

  const testUser = {
    email: `vaibhav${Date.now()}@example.com`,
    password: "123456"
  };

  const testDate = "2026-03-03";

  /**
   * Clean DB before tests
   */
  beforeAll(async () => {
    await prisma.activityLog.deleteMany();
    await prisma.dailySummary.deleteMany();
    await prisma.user.deleteMany();
  });

  /**
   * Disconnect Prisma after tests
   */
  afterAll(async () => {
    await prisma.$disconnect();
  });

  /**
   * REGISTER
   */
  test("Should register a new user", async () => {
    const res = await request(app)
      .post("/api/auth/register")
      .send(testUser);

    expect([200, 201]).toContain(res.statusCode);
    expect(res.body).toHaveProperty("message");
  });

  /**
   * LOGIN
   */
  test("Should login and return token", async () => {
    const res = await request(app)
      .post("/api/auth/login")
      .send(testUser);

    expect(res.statusCode).toBe(200);
    expect(res.body).toHaveProperty("token");

    token = res.body.token;
  });

  /**
   * PROTECTED ROUTE
   */
  test("Should access protected route with valid token", async () => {
    const res = await request(app)
      .get("/api/auth/me")
      .set("Authorization", `Bearer ${token}`);

    expect(res.statusCode).toBe(200);
    expect(res.body).toHaveProperty("email", testUser.email);
  });

  /**
   * LOG ACTIVITIES (needed for summary)
   */
  test("Should log productive and distracting activity", async () => {
    // Productive
    await request(app)
      .post("/api/activity")
      .set("Authorization", `Bearer ${token}`)
      .send({
        domain: "github.com",
        title: "Coding",
        duration_seconds: 600,
        timestamp: `${testDate}T10:00:00Z`
      });

    // Distracting
    await request(app)
      .post("/api/activity")
      .set("Authorization", `Bearer ${token}`)
      .send({
        domain: "youtube.com",
        title: "Random Video",
        duration_seconds: 300,
        timestamp: `${testDate}T11:00:00Z`
      });
  });

  /**
   * DAILY SUMMARY
   */
  test("Should return daily summary correctly", async () => {
    const res = await request(app)
      .get(`/api/summary/daily?date=${testDate}`)
      .set("Authorization", `Bearer ${token}`);

    expect(res.statusCode).toBe(200);

    expect(res.body).toHaveProperty("date", testDate);
    expect(res.body).toHaveProperty("productive_time");
    expect(res.body).toHaveProperty("distracting_time");
    expect(res.body).toHaveProperty("neutral_time");
    expect(res.body).toHaveProperty("learning_time");
    expect(res.body).toHaveProperty("total_time");
    expect(res.body).toHaveProperty("score");

    // Optional stronger assertions (if logic is stable)
    expect(res.body.total_time).toBeGreaterThan(0);
  });
});
