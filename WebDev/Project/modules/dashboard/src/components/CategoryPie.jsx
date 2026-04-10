import { Doughnut } from 'react-chartjs-2';
import {
  Chart as ChartJS,
  ArcElement,
  Tooltip,
  Legend,
} from 'chart.js';
import { getCategoryColor } from '../utils';

ChartJS.register(ArcElement, Tooltip, Legend);

export default function CategoryPie({ data }) {
  if (!data?.categories?.length) {
    return <div className="no-data">No category data available</div>;
  }

  const chartData = {
    labels: data.categories.map(c =>
      c.category.charAt(0).toUpperCase() + c.category.slice(1)
    ),
    datasets: [
      {
        data: data.categories.map(c => c.total_seconds),
        backgroundColor: data.categories.map(c =>
          getCategoryColor(c.category) + '33'
        ),
        borderColor: data.categories.map(c =>
          getCategoryColor(c.category)
        ),
        borderWidth: 2,
        hoverOffset: 8,
      },
    ],
  };

  const options = {
    responsive: true,
    maintainAspectRatio: false,
    cutout: '65%',
    plugins: {
      legend: {
        position: 'bottom',
        labels: {
          color: '#94a3b8',
          font: { size: 12, family: 'Inter' },
          padding: 16,
          usePointStyle: true,
          pointStyleWidth: 10,
        },
      },
      tooltip: {
        backgroundColor: 'rgba(17, 24, 39, 0.95)',
        titleColor: '#f1f5f9',
        bodyColor: '#94a3b8',
        borderColor: 'rgba(255,255,255,0.1)',
        borderWidth: 1,
        padding: 12,
        callbacks: {
          label: (ctx) => {
            const cat = data.categories[ctx.dataIndex];
            const mins = Math.round(cat.total_seconds / 60);
            return ` ${mins}m (${cat.percentage}%)`;
          },
        },
      },
    },
  };

  return (
    <div style={{ height: '280px' }}>
      <Doughnut data={chartData} options={options} />
    </div>
  );
}
