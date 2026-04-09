import { useEffect, useState } from 'react';

export default function ScoreCard({ data }) {
  const [offset, setOffset] = useState(440);

  const score = data?.score ?? 0;
  const percentage = Math.round(score * 100);
  const circumference = 2 * Math.PI * 58; // r=58
  const targetOffset = circumference - (score * circumference);

  useEffect(() => {
    // Animate the ring on mount/update
    const timer = setTimeout(() => setOffset(targetOffset), 100);
    return () => clearTimeout(timer);
  }, [targetOffset]);

  // Color based on score
  const getScoreColor = () => {
    if (score >= 0.7) return '#22c55e';
    if (score >= 0.4) return '#f59e0b';
    return '#ef4444';
  };

  return (
    <div className="score-card-inner">
      <div className="score-ring">
        <svg width="140" height="140" viewBox="0 0 140 140">
          <circle
            className="score-ring-bg"
            cx="70" cy="70" r="58"
          />
          <circle
            className="score-ring-fill"
            cx="70" cy="70" r="58"
            stroke={getScoreColor()}
            strokeDasharray={circumference}
            strokeDashoffset={offset}
          />
        </svg>
        <div className="score-value">
          <div className="score-number" style={{ color: getScoreColor() }}>
            {percentage}
          </div>
          <div className="score-label">Score</div>
        </div>
      </div>

      <div style={{ textAlign: 'center' }}>
        <div style={{ fontSize: '0.8rem', color: 'var(--text-muted)' }}>
          {score >= 0.7 ? '🔥 Highly Productive' :
           score >= 0.4 ? '⚡ Moderately Productive' :
           '😴 Low Productivity'}
        </div>
      </div>
    </div>
  );
}
