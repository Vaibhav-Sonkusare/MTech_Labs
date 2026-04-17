const MQTT_BROKER = "broker.hivemq.com";
const MQTT_PORT = 8884; // WebSockets secure port
const CLIENT_ID = "dashboard_" + Math.random().toString(16).substr(2, 8);
const BASE_TOPIC = "smart_geyser";

let client = new Paho.MQTT.Client(MQTT_BROKER, MQTT_PORT, "/mqtt", CLIENT_ID);

client.onConnectionLost = onConnectionLost;
client.onMessageArrived = onMessageArrived;

// Dynamic geyser registry — populated by device registration events
let geysers = {};

function createGeyserCard(gid, name) {
    if (geysers[gid]) return; // Already exists

    geysers[gid] = {
        temp: 25,
        flow: 0,
        state: 'OFF',
        name: name || `Geyser ${gid}`,
        spinners: { d: 0, h: 0, m: 0 },
        alerts: []
    };

    window[`toggleControls_${gid}`] = function() {
        const cp = document.getElementById(`controls-${gid}`);
        cp.classList.toggle('show');
    };

    window[`spin_${gid}`] = function(unit, delta) {
        let val = geysers[gid].spinners[unit];
        val += delta;
        if (unit === 'm' && val >= 60) { val = 0; geysers[gid].spinners['h']++; }
        if (unit === 'm' && val < 0) { val = 59; geysers[gid].spinners['h']--; }
        if (unit === 'h' && val >= 24) { val = 0; geysers[gid].spinners['d']++; }
        if (unit === 'h' && val < 0) { val = 23; geysers[gid].spinners['d']--; }
        
        // Clamp
        if (geysers[gid].spinners['d'] < 0) geysers[gid].spinners['d'] = 0;
        if (geysers[gid].spinners['h'] < 0) geysers[gid].spinners['h'] = 0;
        if (geysers[gid].spinners['m'] < 0) geysers[gid].spinners['m'] = 0;
        
        // Max 14 days
        if (geysers[gid].spinners['d'] > 14) geysers[gid].spinners['d'] = 14;
        
        geysers[gid].spinners[unit] = val;
        
        document.getElementById(`spin-d-${gid}`).innerText = geysers[gid].spinners['d'];
        document.getElementById(`spin-h-${gid}`).innerText = geysers[gid].spinners['h'];
        document.getElementById(`spin-m-${gid}`).innerText = geysers[gid].spinners['m'];
    };

    window[`sendControl_${gid}`] = function(action) {
        let payload = {};
        const totalMinutes = (geysers[gid].spinners['d'] * 24 * 60) + (geysers[gid].spinners['h'] * 60) + geysers[gid].spinners['m'];
        
        if (action === 'stop' && totalMinutes > 0) {
            payload = { stop_minutes: totalMinutes };
            addLog(`⏳ ${geysers[gid].name}: Manual STOP requested for ${totalMinutes} mins.`, 'prediction');
        } else if (action === 'request' && totalMinutes > 0) {
            payload = { request_minutes: totalMinutes };
            addLog(`♨️ ${geysers[gid].name}: Hot water requested IN ${totalMinutes} mins.`, 'prediction');
        } else if (action === 'cancel') {
            payload = { cancel: true };
            addLog(`🛑 ${geysers[gid].name}: Cancelled all overrides.`, 'prediction');
        } else {
            return; // 0 time specified for stop/request
        }

        const msg = new Paho.MQTT.Message(JSON.stringify(payload));
        msg.destinationName = `${BASE_TOPIC}/geyser/${gid}/control`;
        client.send(msg);
        
        // Hide panel
        document.getElementById(`controls-${gid}`).classList.remove('show');
    };

    const grid = document.getElementById("geysers-grid");
    const card = document.createElement("div");
    card.className = "card";
    card.id = `geyser-${gid}`;

    card.innerHTML = `
        <div class="card-header">
            <h3>${geysers[gid].name}</h3>
            <span class="geyser-state state-off" id="state-${gid}">OFF</span>
        </div>
        <div class="metric">
            <span class="metric-label">Water Temp</span>
            <span class="metric-value"><span id="temp-${gid}">25</span>°C</span>
        </div>
        <div class="temp-indicator">
            <div class="temp-fill" id="temp-fill-${gid}"></div>
        </div>
        <div class="metric mt-2">
            <span class="metric-label">Flow Rate</span>
            <span class="metric-value"><span id="flow-${gid}">0.0</span> L/min</span>
        </div>
        <div class="metric mt-2">
            <span class="metric-label">ML Prediction</span>
            <span class="metric-value" style="font-size: 1rem" id="pred-${gid}">-</span>
        </div>
        <div class="alert-container" id="alerts-${gid}"></div>
        <button class="controls-toggle" onclick="toggleControls_${gid}()">⚙️ Overrides & Controls</button>
        <div class="control-panel" id="controls-${gid}">
            <div id="override-status-${gid}" style="display:none;" class="override-status"></div>
            <div class="spinner-container">
                <div class="spinner-group">
                    <button class="spinner-btn" onclick="spin_${gid}('d', 1)">▲</button>
                    <span class="spinner-val" id="spin-d-${gid}">0</span>
                    <span class="spinner-label">Days</span>
                    <button class="spinner-btn" onclick="spin_${gid}('d', -1)">▼</button>
                </div>
                <div style="font-size: 1.5rem; color: var(--text-secondary)">:</div>
                <div class="spinner-group">
                    <button class="spinner-btn" onclick="spin_${gid}('h', 1)">▲</button>
                    <span class="spinner-val" id="spin-h-${gid}">0</span>
                    <span class="spinner-label">Hours</span>
                    <button class="spinner-btn" onclick="spin_${gid}('h', -1)">▼</button>
                </div>
                <div style="font-size: 1.5rem; color: var(--text-secondary)">:</div>
                <div class="spinner-group">
                    <button class="spinner-btn" onclick="spin_${gid}('m', 15)">▲</button>
                    <span class="spinner-val" id="spin-m-${gid}">0</span>
                    <span class="spinner-label">Mins</span>
                    <button class="spinner-btn" onclick="spin_${gid}('m', -15)">▼</button>
                </div>
            </div>
            <div class="action-buttons">
                <button class="btn btn-stop" id="btn-stop-${gid}" onclick="sendControl_${gid}('stop')">⏹️ Stop Heater</button>
                <button class="btn btn-request" id="btn-request-${gid}" onclick="sendControl_${gid}('request')">♨️ Set Request</button>
                <button class="btn btn-cancel" id="btn-cancel-${gid}" onclick="sendControl_${gid}('cancel')">❌ Cancel Overrides</button>
            </div>
        </div>
    `;
    grid.appendChild(card);
    addLog(`🆕 ${geysers[gid].name} (ID: ${gid}) connected.`);
}

function updateGeyserUI(id) {
    const g = geysers[id];
    if (!g) return;

    const tempEl = document.getElementById(`temp-${id}`);
    const flowEl = document.getElementById(`flow-${id}`);
    const stateEl = document.getElementById(`state-${id}`);
    const fillEl = document.getElementById(`temp-fill-${id}`);

    if (tempEl) tempEl.innerText = parseFloat(g.temp).toFixed(1);
    if (flowEl) flowEl.innerText = parseFloat(g.flow).toFixed(1);

    if (stateEl) {
        stateEl.innerText = g.state;
        stateEl.className = `geyser-state state-${g.state.toLowerCase()}`;
    }

    if (fillEl) {
        let pct = ((g.temp - 20) / (75 - 20)) * 100;
        pct = Math.max(0, Math.min(100, pct));
        fillEl.style.width = `${pct}%`;
    }
}

function addLog(msg, type = 'normal') {
    const container = document.getElementById('logs-container');
    const log = document.createElement('div');
    log.className = `log-entry ${type}`;
    const time = new Date().toLocaleTimeString();
    log.innerHTML = `<span style="color:#94A3B8">[${time}]</span> ${msg}`;
    container.insertBefore(log, container.firstChild);

    if (container.children.length > 50) {
        container.removeChild(container.lastChild);
    }
}

function onConnect() {
    console.log("Connected to MQTT Broker");
    const status = document.getElementById('connection-status');
    status.innerText = "Connected";
    status.className = "status connected";

    // Subscribe to all relevant topics
    client.subscribe(`${BASE_TOPIC}/geyser/+/data`);
    client.subscribe(`${BASE_TOPIC}/dashboard/inference`);
    client.subscribe(`${BASE_TOPIC}/dashboard/devices`);
    addLog("Connected to Cloud Server. Waiting for devices...");
}

function onConnectionLost(responseObject) {
    if (responseObject.errorCode !== 0) {
        console.log("Connection lost: " + responseObject.errorMessage);
        const status = document.getElementById('connection-status');
        status.innerText = "Disconnected";
        status.className = "status";
        addLog(`Connection lost: ${responseObject.errorMessage}`, 'normal');

        setTimeout(() => {
            console.log("Reconnecting...");
            client.connect({ onSuccess: onConnect, useSSL: true });
        }, 5000);
    }
}

function onMessageArrived(message) {
    try {
        const topic = message.destinationName;
        const payload = JSON.parse(message.payloadString);
        const parts = topic.split('/');

        // ── Device registration events ──────────────────────────────
        if (topic.endsWith('/devices')) {
            if (payload.event === 'device_registered') {
                createGeyserCard(payload.geyser_id, payload.name);
            } else if (payload.event === 'device_deleted') {
                addLog(`🗑️ Device ${payload.mac} removed from registry.`);
            }
            return;
        }

        // ── Sensor data ─────────────────────────────────────────────
        if (topic.endsWith('/data')) {
            const gid = parts[2];
            // Auto-create card if we haven't seen this geyser yet
            if (!geysers[gid]) {
                createGeyserCard(gid, `Geyser ${gid}`);
            }
            geysers[gid].temp = payload.water_temp;
            geysers[gid].flow = payload.flow_rate;
            updateGeyserUI(gid);

            if (payload.hour !== undefined && payload.minute !== undefined) {
                const hh = payload.hour.toString().padStart(2, '0');
                const mm = payload.minute.toString().padStart(2, '0');
                document.getElementById('sim-time').innerText = `Time: ${hh}:${mm}`;
            }
            return;
        }

        // ── Inference results ───────────────────────────────────────
        if (topic.endsWith('/inference')) {
            const gid = payload.geyser_id;
            if (!geysers[gid]) createGeyserCard(gid, `Geyser ${gid}`);

            geysers[gid].state = payload.command;

            const predEl = document.getElementById(`pred-${gid}`);
            if (predEl) {
                predEl.innerText = payload.ml_prediction === 1 ? 'DEMAND (High)' : 'IDLE';
                predEl.style.color = payload.ml_prediction === 1 ? 'var(--warning)' : 'var(--text-secondary)';
            }

            updateGeyserUI(gid);
            addLog(`⚙️ Geyser ${gid}: ML Predicted Demand=${payload.ml_prediction} (Conf: ${(payload.confidence * 100).toFixed(1)}%). Command Sent -> ${payload.command}`, 'prediction');
            
            // Handle Alerts
            const alertsCont = document.getElementById(`alerts-${gid}`);
            if (alertsCont && payload.alerts) {
                let htmlStr = '';
                payload.alerts.forEach(al => {
                    if (al === 'low_water') {
                        htmlStr += `<div class="alert-badge alert-low-water">⚠️ LOW WATER LEVEL</div>`;
                        if (!geysers[gid].alerts.includes('low_water')) {
                            addLog(`⚠️ Geyser ${gid} reported LOW water level! Heater disabled.`, 'danger');
                        }
                    } else if (al === 'mains_off') {
                        htmlStr += `<div class="alert-badge alert-mains-off">⚡ MAINS SWITCH IS OFF</div>`;
                        if (!geysers[gid].alerts.includes('mains_off')) {
                            addLog(`⚡ Geyser ${gid} is predicted ON but no current detected. Check Mains Switch!`, 'danger');
                        }
                    }
                });
                alertsCont.innerHTML = htmlStr;
                geysers[gid].alerts = payload.alerts; // Update tracked state
            }
            
            // Handle Overrides Lock UI
            const statusBox = document.getElementById(`override-status-${gid}`);
            const btnStop = document.getElementById(`btn-stop-${gid}`);
            const btnReq = document.getElementById(`btn-request-${gid}`);
            
            if (payload.override_active && statusBox && btnStop && btnReq) {
                // Lock the UI
                btnStop.disabled = true;
                btnReq.disabled = true;
                statusBox.style.display = 'block';
                const typ = payload.override_type.toUpperCase();
                const mins = payload.override_remaining_mins;
                statusBox.innerText = `Active Override: ${typ} (${mins} mins remaining)`;
            } else if (statusBox && btnStop && btnReq) {
                // Unlock the UI
                btnStop.disabled = false;
                btnReq.disabled = false;
                statusBox.style.display = 'none';
            }
            
            return;
        }

    } catch (e) {
        console.error("Error parsing message", e);
    }
}

window.onload = () => {
    // No pre-built grid — cards are created dynamically as devices register
    console.log("Connecting...");
    client.connect({ onSuccess: onConnect, useSSL: true });
};
