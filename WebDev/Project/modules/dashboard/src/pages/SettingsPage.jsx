import { useEffect } from 'react';
import { useTheme } from '../context/ThemeContext';
import { isAuthenticated, getUserEmail, logout, getExportUrl } from '../services/api';
import { useNavigate } from 'react-router-dom';
import Sidebar from '../components/Sidebar';

export default function SettingsPage() {
  const { theme, toggleTheme } = useTheme();
  const email = getUserEmail();
  const navigate = useNavigate();

  useEffect(() => {
    if (!isAuthenticated()) {
      navigate('/login');
    }
  }, []);

  const handleLogout = () => {
    logout();
    navigate('/login');
  };

  const handleExport = () => {
    const end = new Date().toISOString().split('T')[0];
    const startDate = new Date();
    startDate.setDate(startDate.getDate() - 6);
    const start = startDate.toISOString().split('T')[0];
    window.open(getExportUrl(start, end), '_blank');
  };

  return (
    <div className="dashboard-layout">
      <Sidebar onLogout={handleLogout} />

      <div className="settings-page">
        <div className="settings-container fade-in">
          <h1>Settings</h1>

          {/* Appearance */}
          <section className="settings-section glass-card">
            <h2>Appearance</h2>
            <div className="settings-row">
              <div>
                <div className="settings-label">Theme</div>
                <div className="settings-desc">Switch between dark and light mode</div>
              </div>
              <button className="theme-toggle-btn" onClick={toggleTheme}>
                {theme === 'dark' ? (
                  <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round">
                    <circle cx="12" cy="12" r="5" />
                    <line x1="12" y1="1" x2="12" y2="3" />
                    <line x1="12" y1="21" x2="12" y2="23" />
                    <line x1="4.22" y1="4.22" x2="5.64" y2="5.64" />
                    <line x1="18.36" y1="18.36" x2="19.78" y2="19.78" />
                    <line x1="1" y1="12" x2="3" y2="12" />
                    <line x1="21" y1="12" x2="23" y2="12" />
                    <line x1="4.22" y1="19.78" x2="5.64" y2="18.36" />
                    <line x1="18.36" y1="5.64" x2="19.78" y2="4.22" />
                  </svg>
                ) : (
                  <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round">
                    <path d="M21 12.79A9 9 0 1111.21 3 7 7 0 0021 12.79z" />
                  </svg>
                )}
                {theme === 'dark' ? 'Switch to Light' : 'Switch to Dark'}
              </button>
            </div>
          </section>

          {/* Account */}
          <section className="settings-section glass-card">
            <h2>Account</h2>
            <div className="settings-row">
              <div>
                <div className="settings-label">Email</div>
                <div className="settings-desc">{email}</div>
              </div>
            </div>
            <div className="settings-row">
              <div>
                <div className="settings-label">Session</div>
                <div className="settings-desc">Sign out of your current session</div>
              </div>
              <button className="logout-btn" onClick={handleLogout}>
                Logout
              </button>
            </div>
          </section>

          {/* Data Management */}
          <section className="settings-section glass-card fade-in">
            <h2>Data Management</h2>
            <div className="settings-row">
              <div>
                <div className="settings-label">Export Data</div>
                <div className="settings-desc">Download your last 7 days of browsing activity as a CSV file</div>
              </div>
              <button className="export-btn" onClick={handleExport} title="Export last 7 days as CSV">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" width="16" height="16" style={{ marginRight: '8px' }}>
                  <path d="M21 15v4a2 2 0 01-2 2H5a2 2 0 01-2-2v-4" />
                  <polyline points="7,10 12,15 17,10" />
                  <line x1="12" y1="15" x2="12" y2="3" />
                </svg>
                CSV
              </button>
            </div>
          </section>

          {/* About */}
          <section className="settings-section glass-card">
            <h2>About</h2>
            <div className="settings-row">
              <div>
                <div className="settings-label">Version</div>
                <div className="settings-desc">1.1.0</div>
              </div>
            </div>
            <div className="settings-row">
              <div>
                <div className="settings-label">Platform</div>
                <div className="settings-desc">Smart Digital Wellbeing & Productivity Analytics</div>
              </div>
            </div>
          </section>
        </div>
      </div>
    </div>
  );
}
