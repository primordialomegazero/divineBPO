// Divine BPO v21.0 — Enterprise Dashboard
// Real-time data via REST API + WebSocket

const API = '/api/stats';
const INFRA = '/api/infra';
const WS_URL = `ws://${location.hostname}:8093`;

let ws = null;

// Fetch stats from API
async function fetchStats() {
  try {
    const res = await fetch(API);
    const data = await res.json();
    updateDashboard(data);
  } catch (e) {
    console.error('Stats fetch error:', e);
  }
}

// Fetch infra status
async function fetchInfra() {
  try {
    const res = await fetch(INFRA);
    const data = await res.json();
    updateInfra(data);
  } catch (e) {
    console.error('Infra fetch error:', e);
  }
}

// Update all dashboard elements
function updateDashboard(data) {
  // Metrics
  setValue('metric-lambda', data.lambda?.toFixed(4) || '0.4812');
  setValue('metric-phi', data.phi?.toFixed(4) || '1.6180');
  setValue('metric-modules', data.modules_active || 14);

  // Cards
  setValue('card-total-value', data.total_tickets || 0);
  setValue('card-ai-value', data.ai_handled || 0);
  setValue('card-human-value', data.human_escalated || 0);
  setValue('card-rate-value', (data.ai_rate || 0) + '%');
  setValue('card-db-value', data.db_tickets || 0);

  // Infra (also from stats)
  updateInfra(data);
}

function updateInfra(data) {
  if (data.redis_ok !== undefined) {
    setInfra('infra-redis', data.redis_ok);
  }
  if (data.rabbitmq_ok !== undefined) {
    setInfra('infra-rabbitmq', data.rabbitmq_ok);
  }
  if (data.swarm_cores !== undefined) {
    document.getElementById('infra-swarm').textContent = data.swarm_cores + ' cores';
    document.getElementById('infra-swarm').className = 'infra-status ' + (data.swarm_cores > 0 ? 'infra-up' : 'infra-down');
  }
  if (data.grc_blocks !== undefined) {
    document.getElementById('infra-grc').textContent = data.grc_blocks + ' blocks';
  }
  if (data.backups !== undefined) {
    document.getElementById('infra-backups').textContent = data.backups + ' saved';
  }
  if (data.uptime !== undefined) {
    document.getElementById('infra-uptime').textContent = formatUptime(data.uptime);
  }
}

function setValue(id, value) {
  const el = document.getElementById(id);
  if (el && el.textContent !== String(value)) {
    el.textContent = value;
    el.classList.add('pulse');
    setTimeout(() => el.classList.remove('pulse'), 300);
  }
}

function setInfra(id, isUp) {
  const el = document.getElementById(id);
  if (el) {
    el.textContent = isUp ? 'UP' : 'DOWN';
    el.className = 'infra-status ' + (isUp ? 'infra-up' : 'infra-down');
  }
}

function formatUptime(seconds) {
  const h = Math.floor(seconds / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  const s = seconds % 60;
  return h > 0 ? `${h}h ${m}m` : `${m}m ${s}s`;
}

// WebSocket for real-time updates
function connectWebSocket() {
  try {
    ws = new WebSocket(WS_URL);
    ws.onopen = () => console.log('WS connected');
    ws.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data);
        updateDashboard(data);
      } catch (e) {}
    };
    ws.onclose = () => { console.log('WS closed, retrying in 5s...'); setTimeout(connectWebSocket, 5000); };
    ws.onerror = () => ws.close();
  } catch (e) {
    console.log('WS unavailable, using polling');
  }
}

// Init
document.addEventListener('DOMContentLoaded', () => {
  fetchStats();
  fetchInfra();
  connectWebSocket();
  setInterval(fetchStats, 10000);
  setInterval(fetchInfra, 30000);
});
