import { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import {
  isAuthenticated,
  getDailySummary,
  getWeeklySummary,
  getCategoryStats,
  getPeakHours,
  getStreak,
} from '../services/api';
import { getToday } from '../utils';

import Sidebar from '../components/Sidebar';
import ScoreCard from '../components/ScoreCard';
import DailyStats from '../components/DailyStats';
import WeeklyTrend from '../components/WeeklyTrend';
import CategoryPie from '../components/CategoryPie';
import PeakHours from '../components/PeakHours';
import TopDomains from '../components/TopDomains';
import LiveTabs from '../components/LiveTabs';

export default function DashboardPage() {
  const navigate = useNavigate();
  const [date, setDate] = useState(getToday());
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');

  const [daily, setDaily] = useState(null);
  const [weekly, setWeekly] = useState(null);
  const [categories, setCategories] = useState(null);
  const [peakHours, setPeakHours] = useState(null);
  const [streak, setStreakData] = useState(null);

  const fetchData = async (selectedDate, isBackgroundRefresh = false) => {
    if (!isBackgroundRefresh) {
      setLoading(true);
    }
    setError('');

    try {
      const [d, w, c, p, s] = await Promise.all([
        getDailySummary(selectedDate),
        getWeeklySummary(selectedDate),
        getCategoryStats(selectedDate),
        getPeakHours(selectedDate),
        getStreak(),
      ]);

      setDaily(d);
      setWeekly(w);
      setCategories(c);
      setPeakHours(p);
      setStreakData(s);
    } catch (err) {
      if (!isBackgroundRefresh) {
        setError(err.message || 'Failed to load data');
      }
      if (err.message === 'Session expired') {
        navigate('/login');
      }
    } finally {
      if (!isBackgroundRefresh) {
        setLoading(false);
      }
    }
  };

  useEffect(() => {
    if (!isAuthenticated()) {
      navigate('/login');
      return;
    }
    fetchData(date);

    // Live update polling
    const interval = setInterval(() => {
      fetchData(date, true);
    }, 10000);

    return () => clearInterval(interval);
  }, [date]);

  const handleLogout = () => navigate('/login');
  const handleDateChange = (e) => setDate(e.target.value);

  return (
    <div className="dashboard-layout">
      <Sidebar onLogout={handleLogout} />

      <main className="dashboard-main">
        <div className="dashboard-header">
          <div className="dashboard-header-left">
            <h1>Dashboard</h1>
            {streak && streak.currentStreak > 0 && (
              <span className="streak-badge" title={`Longest: ${streak.longestStreak} days`}>
                🔥 {streak.currentStreak}-day streak
              </span>
            )}
          </div>
          <div className="dashboard-header-right">
            <div className="date-picker-wrapper">
              <label htmlFor="date-picker">Date</label>
              <input
                id="date-picker"
                type="date"
                className="date-input"
                value={date}
                onChange={handleDateChange}
                max={getToday()}
              />
            </div>
          </div>
        </div>

        {error && (
          <div className="login-error" style={{ marginBottom: '24px' }}>
            {error}
          </div>
        )}

        {loading ? (
          <div className="loading-container">
            <div className="loading-spinner" />
            <div className="loading-text">Loading analytics...</div>
          </div>
        ) : (
          <div className="dashboard-grid">
            <div className="card-score glass-card chart-card fade-in fade-in-delay-1">
              <h3>Productivity Score</h3>
              <ScoreCard data={daily} />
            </div>

            <div className="card-daily-stats glass-card chart-card fade-in fade-in-delay-1">
              <h3>Today&apos;s Overview</h3>
              <DailyStats data={daily} />
            </div>

            <div className="card-weekly glass-card chart-card fade-in fade-in-delay-2">
              <h3>Weekly Trend</h3>
              <WeeklyTrend data={weekly} />
            </div>

            <div className="card-category glass-card chart-card fade-in fade-in-delay-2">
              <h3>Category Distribution</h3>
              <CategoryPie data={categories} />
            </div>

            <div className="card-peak glass-card chart-card fade-in fade-in-delay-3">
              <h3>Activity by Hour</h3>
              <PeakHours data={peakHours} />
            </div>

            <div className="card-domains glass-card chart-card fade-in fade-in-delay-3">
              <h3>Top Domains</h3>
              <TopDomains data={categories} />
            </div>

            <div className="card-live-tabs glass-card chart-card fade-in fade-in-delay-4">
              <h3>Live Tabs</h3>
              <LiveTabs />
            </div>
          </div>
        )}
      </main>
    </div>
  );
}
