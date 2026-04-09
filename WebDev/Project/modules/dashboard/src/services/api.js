const API_BASE = '';

/**
 * Get stored tokens from localStorage.
 */
export function getTokens() {
  const access = localStorage.getItem('access_token');
  const refresh = localStorage.getItem('refresh_token');
  return { access, refresh, access_token: access };
}

/**
 * Store tokens in localStorage.
 */
function setTokens(access, refresh, email) {
  localStorage.setItem('access_token', access);
  if (refresh) localStorage.setItem('refresh_token', refresh);
  if (email) localStorage.setItem('user_email', email);
  
  // Broadcast to extension content script
  window.postMessage({ type: 'WELLBEING_SYNCAuth', token: access, email: email || '' }, '*');
}

/**
 * Clear all auth data.
 */
export function logout() {
  localStorage.removeItem('access_token');
  localStorage.removeItem('refresh_token');
  localStorage.removeItem('user_email');
  
  // Broadcast to extension content script
  window.postMessage({ type: 'WELLBEING_SYNCAuth', token: null }, '*');
}

/**
 * Authenticated fetch wrapper with token refresh.
 */
async function authFetch(url, options = {}) {
  const { access } = getTokens();

  let res = await fetch(url, {
    ...options,
    headers: {
      'Content-Type': 'application/json',
      'Authorization': `Bearer ${access}`,
      ...options.headers,
    },
  });

  // Try refreshing token on 401
  if (res.status === 401) {
    const { refresh } = getTokens();
    if (!refresh) throw new Error('No refresh token');

    const refreshRes = await fetch(`${API_BASE}/api/auth/refresh`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ refresh_token: refresh }),
    });

    if (!refreshRes.ok) {
      logout();
      throw new Error('Session expired');
    }

    const data = await refreshRes.json();
    setTokens(data.access_token);

    // Retry original request
    res = await fetch(url, {
      ...options,
      headers: {
        'Content-Type': 'application/json',
        'Authorization': `Bearer ${data.access_token}`,
        ...options.headers,
      },
    });
  }

  return res;
}

/**
 * Login and store tokens.
 */
export async function login(email, password) {
  const res = await fetch(`${API_BASE}/api/auth/login`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ email, password }),
  });

  if (!res.ok) {
    const err = await res.json();
    throw new Error(err.error || 'Login failed');
  }

  const data = await res.json();
  setTokens(data.access_token, data.refresh_token, email);

  return data;
}

/**
 * Register a new account and store tokens.
 */
export async function register(email, password) {
  const res = await fetch(`${API_BASE}/api/auth/register`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ email, password }),
  });

  if (!res.ok) {
    const err = await res.json();
    throw new Error(err.error || 'Registration failed');
  }

  const data = await res.json();
  setTokens(data.access_token, data.refresh_token, email);

  return data;
}

/**
 * Check if user is authenticated.
 */
export function isAuthenticated() {
  return !!localStorage.getItem('access_token');
}

/**
 * Get stored user email.
 */
export function getUserEmail() {
  return localStorage.getItem('user_email') || '';
}

// ─── API Methods ───

export async function getDailySummary(date) {
  const res = await authFetch(`${API_BASE}/api/summary/daily?date=${date}`);
  if (!res.ok) throw new Error('Failed to fetch daily summary');
  return res.json();
}

export async function getWeeklySummary(date) {
  const res = await authFetch(`${API_BASE}/api/summary/weekly?date=${date}`);
  if (!res.ok) throw new Error('Failed to fetch weekly summary');
  return res.json();
}

export async function getCategoryStats(date) {
  const res = await authFetch(`${API_BASE}/api/stats/categories?date=${date}`);
  if (!res.ok) throw new Error('Failed to fetch category stats');
  return res.json();
}

export async function getPeakHours(date) {
  const res = await authFetch(`${API_BASE}/api/stats/peak-hours?date=${date}`);
  if (!res.ok) throw new Error('Failed to fetch peak hours');
  return res.json();
}

export async function getStreak() {
  const res = await authFetch(`${API_BASE}/api/stats/streak`);
  if (!res.ok) throw new Error('Failed to fetch streak');
  return res.json();
}

export async function getCustomCategories() {
  const res = await authFetch(`${API_BASE}/api/categories/custom`);
  if (!res.ok) throw new Error('Failed to fetch custom categories');
  return res.json();
}

export async function setCustomCategory(domain, category) {
  const res = await authFetch(`${API_BASE}/api/categories/custom`, {
    method: 'PUT',
    body: JSON.stringify({ domain, category }),
  });
  if (!res.ok) throw new Error('Failed to save custom category');
  return res.json();
}

export async function deleteCustomCategory(domain) {
  const res = await authFetch(`${API_BASE}/api/categories/custom/${encodeURIComponent(domain)}`, {
    method: 'DELETE',
  });
  if (!res.ok) throw new Error('Failed to delete custom category');
  return res.json();
}

export function getExportUrl(start, end) {
  const { access_token } = getTokens();
  return `${API_BASE}/api/export/csv?start=${start}&end=${end}&token=${access_token}`;
}
