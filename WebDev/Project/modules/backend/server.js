const http = require('http');
const { WebSocketServer } = require('ws');
const jwt = require('jsonwebtoken');
const app = require('./src/app');
const PORT = process.env.PORT || 3000;

// Create HTTP server and attach Express
const server = http.createServer(app);

// ─── WebSocket Server ───
const wss = new WebSocketServer({ server });

// Track connected clients by userId
// { userId: { dashboards: Set<ws>, extensions: Map<ws, { deviceName, tabs }> } }
const clients = new Map();

function getClientsForUser(userId) {
  if (!clients.has(userId)) {
    clients.set(userId, { dashboards: new Set(), extensions: new Map() });
  }
  return clients.get(userId);
}

// Heartbeat interval to immediately kill ghost connections
const interval = setInterval(() => {
  wss.clients.forEach((ws) => {
    if (ws.isAlive === false) return ws.terminate();
    ws.isAlive = false;
    ws.ping();
  });
}, 30000);

wss.on('close', () => {
  clearInterval(interval);
});

wss.on('connection', (ws, req) => {
  let userId = null;
  let clientType = null;
  let deviceName = null;
  let deviceId = null;

  // Initialize ping/pong state
  ws.isAlive = true;
  ws.on('pong', () => {
    ws.isAlive = true;
  });

  ws.on('message', (raw) => {
    let msg;
    try {
      msg = JSON.parse(raw);
    } catch {
      return;
    }

    if (msg.type === 'ping') {
      ws.isAlive = true;
      return;
    }

    // First message must be auth
    if (msg.type === 'auth') {
      try {
        const decoded = jwt.verify(msg.token, process.env.JWT_SECRET);
        userId = decoded.userId;
        clientType = msg.clientType || 'dashboard';
        deviceName = msg.deviceName || 'Browser';
        deviceId = msg.deviceId; // Can be undefined for dashboards

        const userClients = getClientsForUser(userId);
        
        if (clientType === 'extension') {
          // Prevent ghost tabs: If this deviceId is already connected, kill the old connection
          if (deviceId) {
            for (const [existingWs, data] of userClients.extensions.entries()) {
              if (data.deviceId === deviceId) {
                existingWs.terminate();
                userClients.extensions.delete(existingWs);
              }
            }
          }
          userClients.extensions.set(ws, { deviceId, deviceName, tabs: [], activeTabId: null });
        } else {
          userClients.dashboards.add(ws);
          // Send current tab state to new dashboard client
          broadcastTabsToUser(userId);
        }

        ws.send(JSON.stringify({ type: 'auth_ok' }));
      } catch {
        ws.send(JSON.stringify({ type: 'auth_error' }));
        ws.close();
      }
      return;
    }

    if (!userId) {
      ws.send(JSON.stringify({ type: 'auth_required' }));
      return;
    }

    // Extension sends tab updates → store and broadcast to dashboards
    if (msg.type === 'tabs_update' && clientType === 'extension') {
      const userClients = getClientsForUser(userId);
      const extData = userClients.extensions.get(ws);
      if (extData) {
        extData.tabs = msg.tabs || [];
        extData.activeTabId = msg.activeTabId;
      }
      broadcastTabsToUser(userId);
    }
  });

  ws.on('close', () => {
    if (userId && clientType) {
      const userClients = clients.get(userId);
      if (userClients) {
        if (clientType === 'extension') {
          userClients.extensions.delete(ws);
          // Notify dashboards that a device disconnected
          broadcastTabsToUser(userId);
        } else {
          userClients.dashboards.delete(ws);
        }
        if (userClients.dashboards.size === 0 && userClients.extensions.size === 0) {
          clients.delete(userId);
        }
      }
    }
  });
});

/**
 * Broadcast aggregated tab data from all devices to all dashboards.
 */
function broadcastTabsToUser(userId) {
  const userClients = clients.get(userId);
  if (!userClients) return;

  // Build devices array
  const devices = [];
  for (const [, extData] of userClients.extensions) {
    devices.push({
      deviceName: extData.deviceName,
      deviceId: extData.deviceId,
      tabs: extData.tabs,
      activeTabId: extData.activeTabId,
    });
  }

  const payload = JSON.stringify({
    type: 'tabs_update',
    devices,
    timestamp: Date.now(),
  });

  for (const dashboard of userClients.dashboards) {
    if (dashboard.readyState === 1) {
      dashboard.send(payload);
    }
  }
}

server.listen(PORT, () => {
  console.log(`Server running on port ${PORT} (HTTP + WebSocket)`);
});
