import { useState } from 'react';
import { useNavigate, useLocation } from 'react-router-dom';
import { getUserEmail, logout } from '../services/api';

export default function Sidebar({ onLogout }) {
  const [collapsed, setCollapsed] = useState(false);
  const email = getUserEmail();
  const initial = email ? email[0].toUpperCase() : '?';
  const navigate = useNavigate();
  const location = useLocation();

  const handleLogout = () => {
    logout();
    if (onLogout) onLogout();
    else navigate('/login');
  };

  const isActive = (path) => location.pathname === path;

  return (
    <>
      {/* Mobile overlay */}
      {!collapsed && (
        <div
          className="sidebar-overlay"
          onClick={() => setCollapsed(true)}
        />
      )}

      <aside className={`sidebar ${collapsed ? 'collapsed' : ''}`}>
        {/* Hamburger toggle */}
        <button
          className="sidebar-toggle"
          onClick={() => setCollapsed(!collapsed)}
          aria-label="Toggle sidebar"
        >
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round">
            {collapsed ? (
              <>
                <line x1="3" y1="6" x2="21" y2="6" />
                <line x1="3" y1="12" x2="21" y2="12" />
                <line x1="3" y1="18" x2="21" y2="18" />
              </>
            ) : (
              <>
                <line x1="3" y1="6" x2="21" y2="6" />
                <line x1="3" y1="12" x2="15" y2="12" />
                <line x1="3" y1="18" x2="18" y2="18" />
              </>
            )}
          </svg>
        </button>

        <div className="sidebar-brand">
          <span className="gradient-text">{collapsed ? 'W' : 'Wellbeing'}</span>
        </div>

        <nav className="sidebar-nav">
          <button
            className={`sidebar-link ${isActive('/') ? 'active' : ''}`}
            onClick={() => navigate('/')}
            title="Dashboard"
          >
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round">
              <rect x="3" y="3" width="7" height="7" rx="1" />
              <rect x="14" y="3" width="7" height="7" rx="1" />
              <rect x="3" y="14" width="7" height="7" rx="1" />
              <rect x="14" y="14" width="7" height="7" rx="1" />
            </svg>
            <span className="sidebar-label">Dashboard</span>
          </button>

          <button
            className={`sidebar-link ${isActive('/categories') ? 'active' : ''}`}
            onClick={() => navigate('/categories')}
            title="Categories"
          >
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round">
              <line x1="8" y1="6" x2="21" y2="6" />
              <line x1="8" y1="12" x2="21" y2="12" />
              <line x1="8" y1="18" x2="21" y2="18" />
              <line x1="3" y1="6" x2="3.01" y2="6" />
              <line x1="3" y1="12" x2="3.01" y2="12" />
              <line x1="3" y1="18" x2="3.01" y2="18" />
            </svg>
            <span className="sidebar-label">Categories</span>
          </button>

          <button
            className={`sidebar-link ${isActive('/settings') ? 'active' : ''}`}
            onClick={() => navigate('/settings')}
            title="Settings"
          >
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round">
              <circle cx="12" cy="12" r="3" />
              <path d="M12 1v2M12 21v2M4.22 4.22l1.42 1.42M18.36 18.36l1.42 1.42M1 12h2M21 12h2M4.22 19.78l1.42-1.42M18.36 5.64l1.42-1.42" />
            </svg>
            <span className="sidebar-label">Settings</span>
          </button>
        </nav>

        <div className="sidebar-footer">
          <div className="sidebar-user">
            <div className="sidebar-avatar">{initial}</div>
            {!collapsed && (
              <div className="sidebar-user-info">
                <div className="sidebar-user-email">{email}</div>
              </div>
            )}
          </div>
          <button
            className="sidebar-link"
            onClick={handleLogout}
            style={{ marginTop: '8px' }}
            title="Logout"
          >
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round">
              <path d="M9 21H5a2 2 0 01-2-2V5a2 2 0 012-2h4" />
              <polyline points="16,17 21,12 16,7" />
              <line x1="21" y1="12" x2="9" y2="12" />
            </svg>
            <span className="sidebar-label">Logout</span>
          </button>
        </div>
      </aside>
    </>
  );
}
