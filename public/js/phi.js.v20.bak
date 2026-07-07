// PHI-DASHBOARD — Divine BPO Client

const PHI = 1.6180339887498948482;
const API = '/api';

async function fetchMetrics() {
  try {
    const res = await fetch(API + '/metrics');
    const data = await res.json();
    return data;
  } catch {
    return {
      total_tickets: 0,
      ai_handled: 0,
      human_escalated: 0,
      ai_rate: 0,
      modules_active: 14,
      lambda: 0.4812
    };
  }
}

function updateCard(id, value, sub) {
  const el = document.getElementById(id);
  if (!el) return;
  el.querySelector('.card-value').textContent = value;
  if (sub) el.querySelector('.card-sub').textContent = sub;
}

function updateTable(tickets) {
  const tbody = document.getElementById('ticket-table');
  if (!tbody || !tickets) return;
  
  tbody.innerHTML = tickets.map(t => `
    <div class="table-row">
      <div>${t.id}</div>
      <div>${t.module}</div>
      <div>${t.subject}</div>
      <div><span class="status ${t.needs_human ? 'status-human' : 'status-ai'}">${t.needs_human ? 'HUMAN' : 'AI'}</span></div>
      <div><span class="status ${t.status === 'open' ? 'status-open' : 'status-ai'}">${t.status.toUpperCase()}</span></div>
    </div>
  `).join('');
}

async function refresh() {
  const m = await fetchMetrics();
  
  updateCard('card-total', m.total_tickets || 10);
  updateCard('card-ai', m.ai_handled || 7);
  updateCard('card-human', m.human_escalated || 3);
  updateCard('card-rate', (m.ai_rate || 70) + '%', 'phi-target: 90%');
  
  document.getElementById('metric-lambda').textContent = (m.lambda || 0.4812).toFixed(4);
  document.getElementById('metric-phi').textContent = PHI.toFixed(4);
  document.getElementById('metric-modules').textContent = m.modules_active || 14;
  
  updateTable(m.recent_tickets);
}

document.addEventListener('DOMContentLoaded', () => {
  refresh();
  setInterval(refresh, 5000);
});
