import { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import {
  isAuthenticated,
  getCategoryStats,
  getCustomCategories,
  setCustomCategory,
  deleteCustomCategory,
} from '../services/api';
import { getToday, formatTime } from '../utils';
import Sidebar from '../components/Sidebar';

const CATEGORY_META = {
  productive:  { icon: '🎯', color: '#22c55e', label: 'Productive' },
  learning:    { icon: '📚', color: '#8b5cf6', label: 'Learning' },
  neutral:     { icon: '🔘', color: '#64748b', label: 'Neutral' },
  distracting: { icon: '📱', color: '#ef4444', label: 'Distracting' },
};

const CATEGORY_OPTIONS = ['productive', 'learning', 'neutral', 'distracting'];

export default function CategoriesPage() {
  const navigate = useNavigate();
  const [date, setDate] = useState(getToday());
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');
  const [categories, setCategories] = useState(null);
  const [overrides, setOverrides] = useState({});

  const fetchData = async (selectedDate) => {
    setLoading(true);
    setError('');
    try {
      const [data, customData] = await Promise.all([
        getCategoryStats(selectedDate),
        getCustomCategories(),
      ]);
      setCategories(data);
      // Build overrides map: { domain: category }
      const map = {};
      (customData.overrides || []).forEach(o => { map[o.domain] = o.category; });
      setOverrides(map);
    } catch (err) {
      setError(err.message || 'Failed to load data');
      if (err.message === 'Session expired') navigate('/login');
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    if (!isAuthenticated()) { navigate('/login'); return; }
    fetchData(date);
  }, [date]);

  const handleLogout = () => navigate('/login');

  const handleCategoryChange = async (domain, newCategory) => {
    try {
      // Normalize domain
      let d = domain.toLowerCase().trim();
      if (d.startsWith('www.')) d = d.slice(4);

      if (newCategory === 'auto') {
        await deleteCustomCategory(d);
        setOverrides(prev => {
          const next = { ...prev };
          delete next[d];
          return next;
        });
      } else {
        await setCustomCategory(d, newCategory);
        setOverrides(prev => ({ ...prev, [d]: newCategory }));
      }
    } catch (err) {
      console.error(err);
    }
  };

  const orderedKeys = ['productive', 'learning', 'neutral', 'distracting'];

  const categoryMap = {};
  if (categories?.categories) {
    for (const cat of categories.categories) {
      categoryMap[cat.category] = cat;
    }
  }

  return (
    <div className="dashboard-layout">
      <Sidebar onLogout={handleLogout} />
      <main className="dashboard-main">
        <div className="dashboard-header">
          <h1>Categories</h1>
          <div className="date-picker-wrapper">
            <label htmlFor="cat-date">Date</label>
            <input
              id="cat-date"
              type="date"
              className="date-input"
              value={date}
              onChange={(e) => setDate(e.target.value)}
              max={getToday()}
            />
          </div>
        </div>

        {error && (
          <div className="login-error" style={{ marginBottom: '24px' }}>{error}</div>
        )}

        {loading ? (
          <div className="loading-container">
            <div className="loading-spinner" />
            <div className="loading-text">Loading categories...</div>
          </div>
        ) : (
          <div className="categories-grid">
            {orderedKeys.map((key, idx) => {
              const meta = CATEGORY_META[key];
              const cat = categoryMap[key];
              const domains = cat?.top_domains || [];
              const totalSecs = cat?.total_seconds || 0;

              return (
                <div
                  key={key}
                  className={`category-panel glass-card fade-in fade-in-delay-${idx + 1}`}
                >
                  <div className="category-panel-header">
                    <span className="category-panel-icon">{meta.icon}</span>
                    <span className="category-panel-title">{meta.label}</span>
                    <span
                      className="category-panel-badge"
                      style={{ backgroundColor: meta.color + '22', color: meta.color }}
                    >
                      {formatTime(totalSecs)}
                    </span>
                  </div>

                  {domains.length === 0 ? (
                    <div className="category-panel-empty">No data for this category</div>
                  ) : (
                    <div className="category-panel-list">
                      {domains.map((d, i) => {
                        const pct = totalSecs > 0 ? Math.round((d.seconds / totalSecs) * 100) : 0;
                        const domainNorm = d.domain.toLowerCase().replace(/^www\./, '');
                        const isCustom = overrides[domainNorm] !== undefined;

                        return (
                          <div key={i} className="category-domain-item">
                            <span className="category-domain-rank" style={{ color: meta.color }}>
                              {i + 1}
                            </span>
                            <img
                              className="category-domain-favicon"
                              src={`https://www.google.com/s2/favicons?domain=${d.domain}&sz=32`}
                              alt=""
                              onError={(e) => { e.target.style.display = 'none'; }}
                            />
                            <span className="category-domain-name">
                              {d.domain}
                              {isCustom && <span className="custom-badge">custom</span>}
                            </span>
                            <span className="category-domain-time">{formatTime(d.seconds)}</span>
                            <select
                              className="category-select"
                              value={overrides[domainNorm] || 'auto'}
                              onChange={(e) => handleCategoryChange(d.domain, e.target.value)}
                            >
                              <option value="auto">Auto</option>
                              {CATEGORY_OPTIONS.map(opt => (
                                <option key={opt} value={opt}>
                                  {opt.charAt(0).toUpperCase() + opt.slice(1)}
                                </option>
                              ))}
                            </select>
                          </div>
                        );
                      })}
                    </div>
                  )}
                </div>
              );
            })}
          </div>
        )}
      </main>
    </div>
  );
}
