import { useState, useEffect, useRef } from 'react';
import { getTokens } from '../services/api';

export default function LiveTabs() {
  const [devices, setDevices] = useState([]);
  const [connected, setConnected] = useState(false);
  const [collapsed, setCollapsed] = useState({});
  const wsRef = useRef(null);

  useEffect(() => {
    let reconnectTimer;

    function connect() {
      const { access_token } = getTokens();
      if (!access_token) return;

      const protocol = location.protocol === 'https:' ? 'wss' : 'ws';
      const ws = new WebSocket(`${protocol}://${location.host}`);
      wsRef.current = ws;

      ws.onopen = () => {
        ws.send(JSON.stringify({
          type: 'auth',
          token: access_token,
          clientType: 'dashboard',
        }));
      };

      ws.onmessage = (e) => {
        const msg = JSON.parse(e.data);
        if (msg.type === 'auth_ok') {
          setConnected(true);
        } else if (msg.type === 'tabs_update') {
          setDevices(msg.devices || []);
        }
      };

      ws.onclose = () => {
        setConnected(false);
        wsRef.current = null;
        reconnectTimer = setTimeout(connect, 5000);
      };

      ws.onerror = () => ws.close();
    }

    connect();
    return () => {
      clearTimeout(reconnectTimer);
      wsRef.current?.close();
    };
  }, []);

  const toggleDevice = (deviceId, index) => {
    const key = deviceId || index;
    setCollapsed(prev => ({ ...prev, [key]: !prev[key] }));
  };

  const totalTabs = devices.reduce((sum, d) => sum + (d.tabs?.length || 0), 0);

  return (
    <div>
      <div className="live-tabs-header">
        <div className={`live-dot ${connected ? 'connected' : ''}`} />
        <span className="live-label">{connected ? 'Live' : 'Disconnected'}</span>
        <span className="tab-count">
          {totalTabs} tab{totalTabs !== 1 ? 's' : ''} · {devices.length} device{devices.length !== 1 ? 's' : ''}
        </span>
      </div>

      {devices.length === 0 ? (
        <div className="live-tabs-empty">
          {connected ? 'No devices connected' : 'Waiting for connection...'}
        </div>
      ) : (
        <div className="live-tabs-devices">
          {devices.map((device, di) => {
            const key = device.deviceId || di;
            return (
              <div key={key} className="live-device-section">
                <button
                  className="live-device-header"
                  onClick={() => toggleDevice(device.deviceId, di)}
                >
                  <span className="live-device-name">
                    💻 {device.deviceName}
                  </span>
                  <span className="live-device-count">
                    {device.tabs?.length || 0} tabs
                  </span>
                  <span className={`live-device-chevron ${collapsed[key] ? 'collapsed' : ''}`}>
                    ▾
                  </span>
                </button>

                {!collapsed[key] && (
                  <div className="live-tabs-list">
                  {(device.tabs || []).map((tab) => {
                    let domain = '';
                    try { domain = new URL(tab.url).hostname; } catch {}
                    return (
                      <div key={tab.id} className={`live-tab-item ${tab.active ? 'active' : ''}`}>
                        <div className="tab-indicator">
                          {tab.active && <div className="active-dot" />}
                        </div>
                        {tab.favIconUrl && (
                          <img className="tab-favicon" src={tab.favIconUrl} alt="" />
                        )}
                        <div className="tab-info">
                          <div className="tab-title">{tab.title || 'Untitled'}</div>
                          <div className="tab-domain">{domain}</div>
                        </div>
                      </div>
                    );
                  })}
                </div>
              )}
            </div>
          )})}
        </div>
      )}
    </div>
  );
}
