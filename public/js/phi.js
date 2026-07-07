// Divine BPO v22.0 — PQC Enterprise Dashboard
// Full dynamic: REST API + WebSocket + Live Tickets

const API = '/api/stats';
const TICKETS_API = '/api/tickets';
const INFRA = '/api/infra';
const WS_URL = `ws://${location.hostname}:8093`;

let ws = null;

async function fetchStats() {
  try {
    const res = await fetch(API);
    const data = await res.json();
    updateDashboard(data);
  } catch (e) { console.error('Stats:', e); }
}

async function fetchTickets() {
  try {
    const res = await fetch(TICKETS_API);
    const tickets = await res.json();
    updateTickets(tickets);
  } catch (e) { console.error('Tickets:', e); }
}

async function fetchInfra() {
  try {
    const res = await fetch(INFRA);
    const data = await res.json();
    updateInfra(data);
  } catch (e) { console.error('Infra:', e); }
}

function updateDashboard(data) {
  setValue('metric-lambda', data.lambda?.toFixed(4) || '0.4812');
  setValue('metric-phi', data.phi?.toFixed(4) || '1.6180');
  setValue('metric-modules', data.modules_active || 14);
  setValue('card-total-value', data.total_tickets || 0);
  setValue('card-ai-value', data.ai_handled || 0);
  setValue('card-human-value', data.human_escalated || 0);
  setValue('card-rate-value', (data.ai_rate || 0) + '%');
  setValue('card-db-value', data.db_tickets || 0);

  // PQC Status
  if (data.pqc_secured !== undefined) {
    const pqcEl = document.getElementById('card-pqc-value');
    if (pqcEl) {
      pqcEl.textContent = data.pqc_secured ? '🔒 SECURE' : '⚠️ DISABLED';
      pqcEl.style.color = data.pqc_secured ? 'var(--green)' : 'var(--red)';
    }
    const pqcAlg = document.getElementById('card-pqc-algo');
    if (pqcAlg) pqcAlg.textContent = data.pqc_algorithm + ' (' + data.pqc_bits + '-bit)';
  }

  updateInfra(data);
}

function updateInfra(data) {
  if (data.redis_ok !== undefined) setInfra('infra-redis', data.redis_ok);
  if (data.rabbitmq_ok !== undefined) setInfra('infra-rabbitmq', data.rabbitmq_ok);
  if (data.swarm_cores !== undefined) {
    const el = document.getElementById('infra-swarm');
    if (el) { el.textContent = data.swarm_cores + ' cores'; el.className = 'infra-status ' + (data.swarm_cores > 0 ? 'infra-up' : 'infra-down'); }
  }
  if (data.grc_blocks !== undefined) {
    const el = document.getElementById('infra-grc');
    if (el) el.textContent = data.grc_blocks + ' blocks';
  }
  if (data.backups !== undefined) {
    const el = document.getElementById('infra-backups');
    if (el) el.textContent = data.backups + ' saved';
  }
  if (data.uptime !== undefined) {
    const el = document.getElementById('infra-uptime');
    if (el) el.textContent = formatUptime(data.uptime);
  }
}

function updateTickets(tickets) {
  const table = document.getElementById('ticket-table');
  if (!table || !tickets.length) return;

  let html = '';
  tickets.forEach(t => {
    html += `<div class="table-row">
      <div>${t.id}</div>
      <div>${t.module}</div>
      <div>${t.subject}</div>
      <div><span class="status-${t.handler === 'AI' ? 'ai' : 'human'}">${t.handler}</span></div>
      <div><span class="status-${t.status === 'CLOSED' ? 'closed' : 'open'}">${t.status}</span></div>
    </div>`;
  });
  table.innerHTML = html;
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

function connectWebSocket() {
  try {
    ws = new WebSocket(WS_URL);
    ws.onopen = () => console.log('WS live');
    ws.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data);
        updateDashboard(data);
        fetchTickets();
      } catch (e) {}
    };
    ws.onclose = () => setTimeout(connectWebSocket, 5000);
    ws.onerror = () => ws.close();
  } catch (e) {}
}

document.addEventListener('DOMContentLoaded', () => {
  fetchStats();
  fetchTickets();
  fetchInfra();
  connectWebSocket();
  setInterval(fetchStats, 10000);
  setInterval(fetchTickets, 30000);
  setInterval(fetchInfra, 60000);
});
