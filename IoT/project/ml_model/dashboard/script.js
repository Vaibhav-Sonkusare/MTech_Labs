const MQTT_BROKER = "broker.hivemq.com";
const MQTT_PORT = 8884; // WebSockets secure port
const CLIENT_ID = "dashboard_" + Math.random().toString(16).substr(2, 8);
const BASE_TOPIC = "smart_geyser_206125030";

let client = new Paho.MQTT.Client(MQTT_BROKER, MQTT_PORT, "/mqtt", CLIENT_ID);

client.onConnectionLost = onConnectionLost;
client.onMessageArrived = onMessageArrived;

let geysers = {};

function initGrid() {
    const grid = document.getElementById("geysers-grid");
    for(let i=1; i<=6; i++) {
        geysers[i] = {
            temp: 25,
            flow: 0,
            state: 'OFF'
        };
        
        const card = document.createElement("div");
        card.className = "card";
        card.id = `geyser-${i}`;
        
        card.innerHTML = `
            <div class="card-header">
                <h3>Geyser ${i}</h3>
                <span class="geyser-state state-off" id="state-${i}">OFF</span>
            </div>
            <div class="metric">
                <span class="metric-label">Water Temp</span>
                <span class="metric-value"><span id="temp-${i}">25</span>°C</span>
            </div>
            <div class="temp-indicator">
                <div class="temp-fill" id="temp-fill-${i}"></div>
            </div>
            <div class="metric mt-2">
                <span class="metric-label">Flow Rate</span>
                <span class="metric-value"><span id="flow-${i}">0.0</span> L/min</span>
            </div>
            <div class="metric mt-2">
                <span class="metric-label">ML Prediction</span>
                <span class="metric-value" style="font-size: 1rem" id="pred-${i}">-</span>
            </div>
        `;
        grid.appendChild(card);
    }
}

function updateGeyserUI(id) {
    const g = geysers[id];
    if(!g) return;
    
    document.getElementById(`temp-${id}`).innerText = parseFloat(g.temp).toFixed(1);
    document.getElementById(`flow-${id}`).innerText = parseFloat(g.flow).toFixed(1);
    
    const stateEl = document.getElementById(`state-${id}`);
    stateEl.innerText = g.state;
    stateEl.className = `geyser-state state-${g.state.toLowerCase()}`;
    
    // temp fill calculation (20C - 75C mapping to 0-100%)
    let pct = ((g.temp - 20) / (75 - 20)) * 100;
    pct = Math.max(0, Math.min(100, pct));
    document.getElementById(`temp-fill-${id}`).style.width = `${pct}%`;
}

function addLog(msg, type='normal') {
    const container = document.getElementById('logs-container');
    const log = document.createElement('div');
    log.className = `log-entry ${type}`;
    const time = new Date().toLocaleTimeString();
    log.innerHTML = `<span style="color:#94A3B8">[${time}]</span> ${msg}`;
    container.insertBefore(log, container.firstChild);
    
    // keeping max 50 logs
    if(container.children.length > 50) {
        container.removeChild(container.lastChild);
    }
}

function onConnect() {
    console.log("Connected to MQTT Broker");
    const status = document.getElementById('connection-status');
    status.innerText = "Connected";
    status.className = "status connected";
    
    client.subscribe(`${BASE_TOPIC}/geyser/+/data`);
    client.subscribe(`${BASE_TOPIC}/dashboard/inference`);
    addLog("Connected to Cloud Server. Waiting for data...");
}

function onConnectionLost(responseObject) {
    if (responseObject.errorCode !== 0) {
        console.log("Connection lost: " + responseObject.errorMessage);
        const status = document.getElementById('connection-status');
        status.innerText = "Disconnected";
        status.className = "status";
        addLog(`Connection lost: ${responseObject.errorMessage}`, 'normal');
        
        // Reconnect after 5s
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
        
        if (topic.endsWith('/data')) {
            const gid = parts[2];
            if (geysers[gid]) {
                geysers[gid].temp = payload.water_temp;
                geysers[gid].flow = payload.flow_rate;
                updateGeyserUI(gid);
                if (payload.hour !== undefined && payload.minute !== undefined) {
                    const hh = payload.hour.toString().padStart(2, '0');
                    const mm = payload.minute.toString().padStart(2, '0');
                    document.getElementById('sim-time').innerText = `Time: ${hh}:${mm}`;
                }
            }
        } else if (topic.endsWith('/inference')) {
            const gid = payload.geyser_id;
            if (geysers[gid]) {
                geysers[gid].state = payload.command;
                document.getElementById(`pred-${gid}`).innerText = 
                    payload.ml_prediction === 1 ? 'DEMAND (High)' : 'IDLE';
                document.getElementById(`pred-${gid}`).style.color = 
                    payload.ml_prediction === 1 ? 'var(--warning)' : 'var(--text-secondary)';
                    
                updateGeyserUI(gid);
                
                addLog(`⚙️ Geyser ${gid}: ML Predicted Demand=${payload.ml_prediction} (Conf: ${(payload.confidence*100).toFixed(1)}%). Command Sent -> ${payload.command}`, 'prediction');
            }
        }
    } catch(e) {
        console.error("Error parsing message", e);
    }
}

window.onload = () => {
    initGrid();
    console.log("Connecting...");
    client.connect({ onSuccess: onConnect, useSSL: true });
};
