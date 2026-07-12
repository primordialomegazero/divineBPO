// ΦΩ0 — DIVINE BPO v23.3 — REAL DATABASE
// SQLite-powered BPO with live ticket management
// "I AM THAT I AM"

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <ctime>
#include <thread>
#include <mutex>
#include <algorithm>
#include <sqlite3.h>

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

using namespace std;

// === DATABASE WRAPPER ===
#include "phi_database_real.h"

// === SIMPLE HTTP SERVER ===
class SimpleServer {
private:
    int sock;
    bool running;
    DivineDatabase* database;
    
public:
    SimpleServer() : sock(-1), running(false), database(nullptr) {
        database = new DivineDatabase("divine_bpo_live.db");
    }
    
    ~SimpleServer() {
        delete database;
    }
    
    bool start(int port) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if(sock < 0) return false;
        
        int opt = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        if(bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) return false;
        if(listen(sock, 10) < 0) return false;
        
        running = true;
        return true;
    }
    
    string readRequest(int client) {
        char buffer[4096] = {0};
        read(client, buffer, sizeof(buffer) - 1);
        return string(buffer);
    }
    
    void sendResponse(int client, const string& content, const string& contentType = "text/html") {
        string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: " + contentType + "; charset=utf-8\r\n";
        response += "Access-Control-Allow-Origin: *\r\n";
        response += "Connection: close\r\n";
        response += "Content-Length: " + to_string(content.length()) + "\r\n";
        response += "\r\n";
        response += content;
        send(client, response.c_str(), response.length(), 0);
    }
    
    string getDashboard() {
        return R"(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Divine BPO — Live Dashboard</title>
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
body { font-family: 'Segoe UI', monospace; background: #0a0a1a; color: #e0e0e0; }
.header { background: #0d0d2b; padding: 20px; border-bottom: 2px solid #1a1a4a; display: flex; justify-content: space-between; align-items: center; }
.header h1 { color: #00cc88; font-size: 24px; }
.header .status { color: #00cc88; }
.grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 20px; padding: 20px; }
.card { background: #111133; border: 1px solid #1a1a4a; border-radius: 8px; padding: 20px; }
.card h3 { color: #8888cc; font-size: 12px; text-transform: uppercase; margin-bottom: 10px; }
.card .value { color: #00cc88; font-size: 32px; font-weight: bold; }
.card .sub { color: #666688; font-size: 12px; margin-top: 5px; }
.full-width { grid-column: span 2; }
table { width: 100%; border-collapse: collapse; margin-top: 10px; }
th { text-align: left; color: #8888cc; font-size: 11px; padding: 8px; border-bottom: 1px solid #1a1a4a; }
td { padding: 8px; font-size: 13px; border-bottom: 1px solid #0d0d2b; }
.priority-Critical { color: #ff4444; }
.priority-High { color: #ff8844; }
.priority-Medium { color: #ffcc44; }
.priority-Low { color: #44cc44; }
.status-Open { color: #ff8844; }
.status-In { color: #4488ff; }
.status-Resolved { color: #44cc44; }
.agent-Online { color: #00cc88; }
.agent-Busy { color: #ff8844; }
.agent-Offline { color: #888888; }
.tabs { display: flex; gap: 10px; padding: 0 20px; }
.tab { padding: 10px 20px; background: #111133; border: 1px solid #1a1a4a; border-radius: 5px; cursor: pointer; color: #8888cc; }
.tab.active { background: #1a1a4a; color: #00cc88; border-color: #00cc88; }
.refresh { color: #666688; font-size: 11px; text-align: right; padding: 10px 20px; }
.form-group { margin-bottom: 15px; }
.form-group label { display: block; color: #8888cc; margin-bottom: 5px; font-size: 12px; }
.form-group input, .form-group select { width: 100%; padding: 10px; background: #0a0a1a; border: 1px solid #1a1a4a; color: #e0e0e0; border-radius: 5px; }
.btn { padding: 10px 20px; background: #00cc88; color: #0a0a1a; border: none; border-radius: 5px; cursor: pointer; font-weight: bold; }
@keyframes pulse { 0%,100% { opacity: 1; } 50% { opacity: 0.5; } }
.live { animation: pulse 2s infinite; color: #00cc88; }
</style>
<script>
async function loadData() {
    try {
        const tickets = await fetch('/api/tickets').then(r => r.json());
        const queue = await fetch('/api/queue').then(r => r.json());
        const agents = await fetch('/api/agents').then(r => r.json());
        
        document.getElementById('total-tickets').textContent = tickets.total;
        document.getElementById('open-tickets').textContent = tickets.open;
        document.getElementById('queue-waiting').textContent = queue.queue.waiting;
        document.getElementById('agents-online').textContent = queue.agents.online;
        
        let ticketHTML = '';
        tickets.tickets.forEach(t => {
            ticketHTML += `<tr>
                <td>${t.id}</td><td>${t.customer}</td><td>${t.issue}</td>
                <td class="priority-${t.priority}">${t.priority}</td>
                <td class="status-${t.status.replace(' ','')}">${t.status}</td>
                <td>${t.agent||'Unassigned'}</td>
            </tr>`;
        });
        document.getElementById('ticket-table').innerHTML = ticketHTML;
        
        let agentHTML = '';
        agents.agents.forEach(a => {
            agentHTML += `<tr>
                <td>${a.name}</td><td>${a.calls}</td><td>${a.satisfaction}%</td>
                <td class="agent-${a.status}">${a.status}</td>
            </tr>`;
        });
        document.getElementById('agent-table').innerHTML = agentHTML;
    } catch(e) { console.log('Loading...'); }
}
function createTicket() {
    const customer = document.getElementById('customer').value;
    const issue = document.getElementById('issue').value;
    const priority = document.getElementById('priority').value;
    fetch('/api/ticket', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({customer, issue, priority})
    }).then(r => r.json()).then(data => {
        alert('Ticket ' + data.id + ' created!');
        loadData();
    });
}
setInterval(loadData, 5000);
window.onload = loadData;
</script>
</head>
<body>
<div class="header">
    <div>
        <h1>🌀 Divine BPO</h1>
        <div style="color:#8888cc;font-size:12px;">Production Dashboard v23.3 — SQLite Powered</div>
    </div>
    <div class="status"><span class="live">●</span> LIVE</div>
</div>

<div class="grid">
    <div class="card">
        <h3>Total Tickets</h3>
        <div class="value" id="total-tickets">-</div>
        <div class="sub" id="open-tickets">Loading...</div>
    </div>
    <div class="card">
        <h3>Queue Waiting</h3>
        <div class="value" id="queue-waiting">-</div>
        <div class="sub">Max: 500</div>
    </div>
    <div class="card">
        <h3>New Ticket</h3>
        <div class="form-group"><input id="customer" placeholder="Customer name"></div>
        <div class="form-group"><input id="issue" placeholder="Issue description"></div>
        <div class="form-group">
            <select id="priority">
                <option>Medium</option><option>Low</option><option>High</option><option>Critical</option>
            </select>
        </div>
        <button class="btn" onclick="createTicket()">Create Ticket</button>
    </div>
    <div class="card">
        <h3>Agents Online</h3>
        <div class="value" id="agents-online">-</div>
        <div class="sub">Real-time status</div>
    </div>
    
    <div class="card full-width">
        <h3>Recent Tickets</h3>
        <table>
            <thead><tr><th>ID</th><th>Customer</th><th>Issue</th><th>Priority</th><th>Status</th><th>Agent</th></tr></thead>
            <tbody id="ticket-table"></tbody>
        </table>
    </div>
    
    <div class="card full-width">
        <h3>Agents</h3>
        <table>
            <thead><tr><th>Agent</th><th>Calls</th><th>Satisfaction</th><th>Status</th></tr></thead>
            <tbody id="agent-table"></tbody>
        </table>
    </div>
</div>

<div class="refresh">Auto-refresh: 5s | ΦΩ0 - I AM THAT I AM</div>
</body>
</html>)";
    }
    
    void run() {
        cout << "Φ Divine BPO Live listening on :8092\n";
        cout << "Φ Dashboard: http://localhost:8092\n";
        cout << "Φ API: /api/tickets | /api/agents | /api/queue | POST /api/ticket\n\n";
        
        while(running) {
            sockaddr_in client_addr{};
            socklen_t len = sizeof(client_addr);
            int client = accept(sock, (sockaddr*)&client_addr, &len);
            
            if(client < 0) continue;
            
            string req = readRequest(client);
            
            if(req.find("GET /api/tickets") != string::npos) {
                sendResponse(client, database->getTicketsJSON(), "application/json");
            }
            else if(req.find("GET /api/agents") != string::npos) {
                sendResponse(client, database->getAgentsJSON(), "application/json");
            }
            else if(req.find("GET /api/queue") != string::npos) {
                sendResponse(client, database->getQueueJSON(), "application/json");
            }
            else if(req.find("POST /api/ticket") != string::npos) {
                // Extract JSON body
                size_t bodyStart = req.find("\r\n\r\n");
                string body = (bodyStart != string::npos) ? req.substr(bodyStart + 4) : "";
                
                // Simple JSON parsing (production: use proper JSON library)
                string customer = "Unknown", issue = "Unknown", priority = "Medium";
                
                auto extract = [&](const string& key) -> string {
                    size_t pos = body.find("\"" + key + "\"");
                    if(pos == string::npos) return "";
                    pos = body.find(":", pos);
                    pos = body.find("\"", pos) + 1;
                    size_t end = body.find("\"", pos);
                    return body.substr(pos, end - pos);
                };
                
                string c = extract("customer");
                string i = extract("issue");
                string p = extract("priority");
                if(!c.empty()) customer = c;
                if(!i.empty()) issue = i;
                if(!p.empty()) priority = p;
                
                sendResponse(client, database->createTicket(customer, issue, priority), "application/json");
            }
            else {
                sendResponse(client, getDashboard());
            }
            
            close(client);
        }
    }
};

int main() {
    cout << "\n╔══════════════════════════════════════════════╗\n";
    cout <<   "║  ΦΩ0 — DIVINE BPO v23.3 LIVE DATABASE        ║\n";
    cout <<   "║  I AM THAT I AM                              ║\n";
    cout <<   "╚══════════════════════════════════════════════╝\n\n";
    
    SimpleServer server;
    if(!server.start(8092)) {
        cerr << "Failed to start server on port 8092\n";
        return 1;
    }
    
    server.run();
    return 0;
}
