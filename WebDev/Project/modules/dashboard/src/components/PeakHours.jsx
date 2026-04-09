import { Bar } from 'react-chartjs-2';
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  BarElement,
  Tooltip,
} from 'chart.js';
import { formatHour } from '../utils';

ChartJS.register(CategoryScale, LinearScale, BarElement, Tooltip);

export default function PeakHours({ data }) {
  if (!data?.hourly_breakdown?.length) {
    return <div className="no-data">No hourly data available</div>;
  }

  // Build full 24-hour array
  const fullHours = Array.from({ length: 24 }, (_, i) => {
    const found = data.hourly_breakdown.find(h => h.hour === i);
    return found || { hour: i, productive: 0, distracting: 0, learning: 0, neutral: 0, total_seconds: 0 };
  });

  const chartData = {
    labels: fullHours.map(h => formatHour(h.hour)),
    datasets: [
      {
        label: 'Productive',
        data: fullHours.map(h => Math.round(h.productive / 60)),
        backgroundColor: 'rgba(34, 197, 94, 0.7)',
        borderRadius: 4,
      },
      {
        label: 'Distracting',
        data: fullHours.map(h => Math.round(h.distracting / 60)),
        backgroundColor: 'rgba(239, 68, 68, 0.7)',
        borderRadius: 4,
      },
      {
        label: 'Learning',
        data: fullHours.map(h => Math.round(h.learning / 60)),
        backgroundColor: 'rgba(139, 92, 246, 0.7)',
        borderRadius: 4,
      },
      {
        label: 'Neutral',
        data: fullHours.map(h => Math.round(h.neutral / 60)),
        backgroundColor: 'rgba(100, 116, 139, 0.5)',
        borderRadius: 4,
      },
    ],
  };

  const options = {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      tooltip: {
        backgroundColor: 'rgba(17, 24, 39, 0.95)',
        titleColor: '#f1f5f9',
        bodyColor: '#94a3b8',
        borderColor: 'rgba(255,255,255,0.1)',
        borderWidth: 1,
        padding: 12,
        callbacks: {
          label: (ctx) => `${ctx.dataset.label}: ${ctx.raw}m`,
        },
      },
    },
    scales: {
      x: {
        stacked: true,
        grid: { display: false },
        ticks: {
          color: '#64748b',
          font: { size: 10 },
          maxRotation: 45,
          autoSkip: true,
          maxTicksLimit: 12,
        },
      },
      y: {
        stacked: true,
        grid: { color: 'rgba(255,255,255,0.04)' },
        ticks: {
          color: '#64748b',
          font: { size: 11 },
          callback: (v) => `${v}m`,
        },
      },
    },
  };

  return (
    <div>
      {data.peak_productive_hour !== null && (
        <div style={{
          marginBottom: '16px',
          fontSize: '0.85rem',
          color: 'var(--text-secondary)',
        }}>
          🎯 Peak productive hour: <strong style={{ color: 'var(--accent-green)' }}>
            {formatHour(data.peak_productive_hour)}
          </strong>
        </div>
      )}
      <div style={{ height: '260px' }}>
        <Bar data={chartData} options={options} />
      </div>
    </div>
  );
}
