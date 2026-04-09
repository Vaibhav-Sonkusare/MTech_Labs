import { API_BASE } from "./config.js";

const loggedInView = document.getElementById("loggedInView");
const notLoggedInView = document.getElementById("notLoggedInView");

// ─── Format Time ───
function formatTime(seconds) {
  const h = Math.floor(seconds / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  if (h > 0) return `${h}h ${m}m`;
  return `${m}m`;
}

// ─── Fetch Metrics ───
async function fetchMetrics(token) {
  try {
    const today = new Date().toISOString().split('T')[0];
    const res = await fetch(`${API_BASE}/api/summary/daily/${today}`, {
      headers: { 'Authorization': `Bearer ${token}` }
    });
    
    if (res.ok) {
      const summary = await res.json();
      document.getElementById("metricsArea").style.display = "block";
      document.getElementById("popupScore").textContent = `${Math.round((summary.score || 0) * 100)}%`;
      document.getElementById("popupProductive").textContent = formatTime(summary.productiveTime || 0);
      document.getElementById("popupDistracting").textContent = formatTime(summary.distractingTime || 0);
    }
  } catch (err) {
    console.error("Failed to fetch metrics", err);
  }
}

// ─── Check login state ───
async function init() {
  const data = await browser.storage.local.get(["access_token", "user_email"]);
  if (data.access_token) {
    loggedInView.style.display = "block";
    notLoggedInView.style.display = "none";
    document.getElementById("userEmail").textContent = data.user_email || "User";
    
    // Fetch and show metrics
    fetchMetrics(data.access_token);
  } else {
    loggedInView.style.display = "none";
    notLoggedInView.style.display = "block";
  }
}

// ─── Open Dashboard ───
document.getElementById("openDashboard").addEventListener("click", () => {
  browser.tabs.create({ url: `${API_BASE}/` });
  window.close();
});

// ─── Open Login Page ───
document.getElementById("openLoginBtn").addEventListener("click", () => {
  browser.tabs.create({ url: `${API_BASE}/login` });
  window.close();
});

// ─── Logout ───
document.getElementById("logoutBtn").addEventListener("click", async () => {
  await browser.storage.local.remove(["access_token", "refresh_token", "user_email"]);
  init();
});

init();
