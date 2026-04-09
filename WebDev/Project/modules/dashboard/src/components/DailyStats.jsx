import { formatTime } from '../utils';

/**
 * DailyStats — rich stat cards + category progress bars.
 */
export default function DailyStats({ data }) {
  const productive = data?.productive_time || 0;
  const distracting = data?.distracting_time || 0;
  const learning = data?.learning_time || 0;
  const neutral = data?.neutral_time || 0;
  const total = productive + distracting + learning + neutral;

  const pct = (val) => total > 0 ? Math.round((val / total) * 100) : 0;

  const categories = [
    { label: 'Productive', time: productive, color: '#22c55e', icon: '🎯' },
    { label: 'Learning',   time: learning,   color: '#8b5cf6', icon: '📚' },
    { label: 'Distracting', time: distracting, color: '#ef4444', icon: '📱' },
    { label: 'Neutral',    time: neutral,    color: '#64748b', icon: '🔘' },
  ];

  return (
    <div className="daily-stats-rich">
      {/* Stat Cards Row */}
      <div className="stat-cards-row">
        <div className="mini-stat-card">
          <div className="mini-stat-icon">⏱</div>
          <div className="mini-stat-value">{formatTime(total)}</div>
          <div className="mini-stat-label">Total Time</div>
        </div>
        <div className="mini-stat-card">
          <div className="mini-stat-icon">🎯</div>
          <div className="mini-stat-value">{pct(productive)}%</div>
          <div className="mini-stat-label">Productive</div>
        </div>
        <div className="mini-stat-card">
          <div className="mini-stat-icon">📚</div>
          <div className="mini-stat-value">{formatTime(learning)}</div>
          <div className="mini-stat-label">Learning</div>
        </div>
        <div className="mini-stat-card">
          <div className="mini-stat-icon">⚡</div>
          <div className="mini-stat-value">{pct(productive + learning)}%</div>
          <div className="mini-stat-label">Focus Rate</div>
        </div>
      </div>

      {/* Category Breakdown Bars */}
      <div className="category-bars">
        {categories.map(cat => (
          <div key={cat.label} className="category-bar-item">
            <div className="category-bar-header">
              <span className="category-bar-icon">{cat.icon}</span>
              <span className="category-bar-name">{cat.label}</span>
              <span className="category-bar-time">{formatTime(cat.time)}</span>
              <span className="category-bar-pct">{pct(cat.time)}%</span>
            </div>
            <div className="category-bar-track">
              <div
                className="category-bar-fill"
                style={{
                  width: `${pct(cat.time)}%`,
                  backgroundColor: cat.color,
                }}
              />
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}
