import { formatTime } from '../utils';

export default function TopDomains({ data }) {
  if (!data?.categories?.length) {
    return <div className="no-data">No domain data available</div>;
  }

  // Collect all domains across categories, flatten and sort
  const allDomains = [];

  data.categories.forEach(cat => {
    if (cat.top_domains) {
      cat.top_domains.forEach(d => {
        allDomains.push({
          domain: d.domain,
          seconds: d.seconds,
          category: cat.category,
        });
      });
    }
  });

  // Sort by time desc, take top 10
  allDomains.sort((a, b) => b.seconds - a.seconds);
  const topDomains = allDomains.slice(0, 10);
  const maxSeconds = topDomains.length > 0 ? topDomains[0].seconds : 1;

  // Total time for percentage
  const totalSeconds = allDomains.reduce((sum, d) => sum + d.seconds, 0);

  if (topDomains.length === 0) {
    return <div className="no-data">No domain data available</div>;
  }

  const categoryColors = {
    productive: '#22c55e',
    distracting: '#ef4444',
    learning: '#8b5cf6',
    neutral: '#64748b',
  };

  return (
    <div className="domains-list-rich">
      {topDomains.map((item, i) => {
        const pct = totalSeconds > 0 ? Math.round((item.seconds / totalSeconds) * 100) : 0;
        const barWidth = Math.max((item.seconds / maxSeconds) * 100, 4);
        const color = categoryColors[item.category] || '#64748b';

        return (
          <div key={i} className="domain-item-rich">
            <div className="domain-item-header">
              <img
                className="domain-favicon"
                src={`https://www.google.com/s2/favicons?domain=${item.domain}&sz=32`}
                alt=""
                onError={(e) => { e.target.style.display = 'none'; }}
              />
              <span className="domain-name-rich">{item.domain}</span>
              <span className="domain-time-rich">{formatTime(item.seconds)}</span>
              <span className="domain-pct">{pct}%</span>
            </div>
            <div className="domain-bar-track">
              <div
                className="domain-bar-fill"
                style={{ width: `${barWidth}%`, backgroundColor: color }}
              />
            </div>
          </div>
        );
      })}
    </div>
  );
}
