#ifndef WEB_UI_H
#define WEB_UI_H

#include <Arduino.h>

// Helper function to get HTML page
String getWebPage(const String& path) {
  if (path == "/" || path == "/index.html") {
    return R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>OpenWatt P1 Reader</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #f5f5f5; color: #333; line-height: 1.6; }
    .container { max-width: 1200px; margin: 0 auto; padding: 20px; }
    header { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 2rem; border-radius: 10px; margin-bottom: 2rem; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }
    h1 { font-size: 2rem; margin-bottom: 0.5rem; }
    nav { background: white; padding: 1rem; border-radius: 10px; margin-bottom: 2rem; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
    nav a { display: inline-block; padding: 0.5rem 1rem; margin-right: 1rem; text-decoration: none; color: #667eea; border-radius: 5px; transition: background 0.3s; }
    nav a:hover, nav a.active { background: #667eea; color: white; }
    .card { background: white; padding: 1.5rem; border-radius: 10px; margin-bottom: 1.5rem; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
    .card h2 { margin-bottom: 1rem; color: #667eea; }
    .status-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 1rem; margin-top: 1rem; }
    .status-item { padding: 1rem; background: #f8f9fa; border-radius: 8px; border-left: 4px solid #667eea; }
    .status-item strong { display: block; margin-bottom: 0.5rem; color: #666; }
    .status-badge { display: inline-block; padding: 0.25rem 0.75rem; border-radius: 20px; font-size: 0.875rem; font-weight: 600; }
    .status-badge.connected { background: #10b981; color: white; }
    .status-badge.disconnected { background: #ef4444; color: white; }
    .loading { text-align: center; padding: 2rem; color: #666; }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <h1>🔋 OpenWatt P1 Reader</h1>
      <div class="subtitle" id="deviceInfo">Loading...</div>
    </header>
    <nav>
      <a href="/" class="active">Dashboard</a>
      <a href="/live">Live Data</a>
      <a href="/settings">Settings</a>
      <a href="/system">System</a>
    </nav>
    <div id="content">
      <div class="card">
        <h2>Connection Status</h2>
        <div class="status-grid" id="statusGrid">
          <div class="status-item"><strong>WiFi</strong><div><span class="status-badge disconnected" id="wifiStatus">Disconnected</span></div></div>
          <div class="status-item"><strong>Meter</strong><div><span class="status-badge disconnected" id="meterStatus">Disconnected</span></div></div>
          <div class="status-item"><strong>MQTT</strong><div><span class="status-badge disconnected" id="mqttStatus">Disconnected</span></div></div>
        </div>
      </div>
      <div class="card">
        <h2>Latest Meter Reading</h2>
        <div id="latestReading" class="loading">Waiting for meter data...</div>
      </div>
    </div>
  </div>
  <script>
    fetch('/api/system').then(r => r.json()).then(data => {
      document.getElementById('deviceInfo').textContent = data.firmware_version + ' | Device: ' + data.device_id;
    });
    function updateStatus() {
      fetch('/api/state').then(r => r.json()).then(data => {
        const wifiEl = document.getElementById('wifiStatus');
        const meterEl = document.getElementById('meterStatus');
        const mqttEl = document.getElementById('mqttStatus');
        wifiEl.textContent = data.wifi_connected ? 'Connected' : 'Disconnected';
        wifiEl.className = 'status-badge ' + (data.wifi_connected ? 'connected' : 'disconnected');
        meterEl.textContent = data.meter_connected ? 'Connected' : 'Disconnected';
        meterEl.className = 'status-badge ' + (data.meter_connected ? 'connected' : 'disconnected');
        mqttEl.textContent = data.cloud_connected ? 'Connected' : 'Disconnected';
        mqttEl.className = 'status-badge ' + (data.cloud_connected ? 'connected' : 'disconnected');
      });
    }
    updateStatus();
    setInterval(updateStatus, 5000);
  </script>
</body>
</html>)HTML";
  }
  
  if (path == "/live" || path == "/live.html") {
    return R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Live Meter Data - OpenWatt P1 Reader</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #f5f5f5; color: #333; }
    .container { max-width: 1200px; margin: 0 auto; padding: 20px; }
    header { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 2rem; border-radius: 10px; margin-bottom: 2rem; }
    nav { background: white; padding: 1rem; border-radius: 10px; margin-bottom: 2rem; }
    nav a { display: inline-block; padding: 0.5rem 1rem; margin-right: 1rem; text-decoration: none; color: #667eea; border-radius: 5px; }
    nav a:hover, nav a.active { background: #667eea; color: white; }
    .card { background: white; padding: 1.5rem; border-radius: 10px; margin-bottom: 1.5rem; }
    .meter-data { font-family: monospace; background: #1e1e1e; color: #d4d4d4; padding: 1.5rem; border-radius: 8px; }
    .meter-data .row { display: grid; grid-template-columns: 300px 1fr; padding: 0.5rem 0; border-bottom: 1px solid #333; }
    .meter-data .label { color: #9cdcfe; }
    .meter-data .value { color: #4ec9b0; font-weight: bold; }
    .status-indicator { display: inline-block; width: 12px; height: 12px; border-radius: 50%; margin-right: 0.5rem; }
    .status-indicator.connected { background: #10b981; }
    .status-indicator.disconnected { background: #ef4444; }
  </style>
</head>
<body>
  <div class="container">
    <header><h1>📊 Live Meter Data</h1></header>
    <nav>
      <a href="/">Dashboard</a>
      <a href="/live" class="active">Live Data</a>
      <a href="/settings">Settings</a>
      <a href="/system">System</a>
    </nav>
    <div class="card">
      <h2><span class="status-indicator disconnected" id="wsStatus"></span>Connection Status: <span id="wsStatusText">Disconnected</span></h2>
    </div>
    <div class="card">
      <h2>Current Meter Reading</h2>
      <div id="meterData" class="meter-data">Connecting...</div>
    </div>
  </div>
  <script>
    let ws = null;
    function connectWebSocket() {
      const wsUrl = (window.location.protocol === 'https:' ? 'wss:' : 'ws:') + '//' + window.location.host + '/api/live';
      ws = new WebSocket(wsUrl);
      ws.onopen = () => {
        document.getElementById('wsStatus').className = 'status-indicator connected';
        document.getElementById('wsStatusText').textContent = 'Connected';
      };
      ws.onmessage = (e) => {
        const data = JSON.parse(e.data);
        document.getElementById('meterData').innerHTML = 
          '<div class="row"><span class="label">Equipment ID:</span><span class="value">' + (data['0-0:96.1.1'] || 'N/A') + '</span></div>' +
          '<div class="row"><span class="label">Timestamp:</span><span class="value">' + (data['0-0:1.0.0'] || 'N/A') + '</span></div>' +
          '<div class="row"><span class="label">Power Consumed:</span><span class="value">' + (data['1-0:1.7.0'] || 0) + ' kW</span></div>' +
          '<div class="row"><span class="label">Power Produced:</span><span class="value">' + (data['1-0:2.7.0'] || 0) + ' kW</span></div>' +
          '<div class="row"><span class="label">Consumption T1:</span><span class="value">' + (data['1-0:1.8.1'] || 0) + ' kWh</span></div>' +
          '<div class="row"><span class="label">Consumption T2:</span><span class="value">' + (data['1-0:1.8.2'] || 0) + ' kWh</span></div>';
      };
      ws.onclose = () => {
        document.getElementById('wsStatus').className = 'status-indicator disconnected';
        document.getElementById('wsStatusText').textContent = 'Disconnected';
        setTimeout(connectWebSocket, 3000);
      };
    }
    connectWebSocket();
  </script>
</body>
</html>)HTML";
  }
  
  if (path == "/settings" || path == "/settings.html") {
    return R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Settings - OpenWatt P1 Reader</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #f5f5f5; color: #333; }
    .container { max-width: 800px; margin: 0 auto; padding: 20px; }
    header { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 2rem; border-radius: 10px; margin-bottom: 2rem; }
    nav { background: white; padding: 1rem; border-radius: 10px; margin-bottom: 2rem; }
    nav a { display: inline-block; padding: 0.5rem 1rem; margin-right: 1rem; text-decoration: none; color: #667eea; border-radius: 5px; }
    nav a:hover, nav a.active { background: #667eea; color: white; }
    .card { background: white; padding: 1.5rem; border-radius: 10px; margin-bottom: 1.5rem; }
    .form-group { margin-bottom: 1rem; }
    .form-group label { display: block; margin-bottom: 0.5rem; font-weight: 600; }
    .form-group input { width: 100%; padding: 0.75rem; border: 1px solid #ddd; border-radius: 5px; }
    .btn { padding: 0.75rem 1.5rem; background: #667eea; color: white; border: none; border-radius: 5px; cursor: pointer; }
    .success { background: #d4edda; color: #155724; padding: 1rem; border-radius: 5px; margin: 1rem 0; }
  </style>
</head>
<body>
  <div class="container">
    <header><h1>⚙️ Settings</h1></header>
    <nav>
      <a href="/">Dashboard</a>
      <a href="/live">Live Data</a>
      <a href="/settings" class="active">Settings</a>
      <a href="/system">System</a>
    </nav>
    <div id="message"></div>
    <div class="card">
      <h2>WiFi Configuration</h2>
      <form id="wifiForm">
        <div class="form-group">
          <button type="button" class="btn" onclick="scanWiFi()">Scan Networks</button>
          <div id="wifiList" style="margin-top: 1rem;"></div>
        </div>
        <div class="form-group">
          <label>SSID</label>
          <input type="text" id="wifiSSID" required>
        </div>
        <div class="form-group">
          <label>Password</label>
          <input type="password" id="wifiPassword">
        </div>
        <button type="submit" class="btn">Save WiFi Settings</button>
      </form>
    </div>
    <div class="card">
      <h2>MQTT Configuration</h2>
      <form id="mqttForm">
        <div class="form-group">
          <label>MQTT Broker Host</label>
          <input type="text" id="mqttHost">
        </div>
        <div class="form-group">
          <label>MQTT Port</label>
          <input type="number" id="mqttPort" value="1883">
        </div>
        <div class="form-group">
          <label>MQTT Topic</label>
          <input type="text" id="mqttTopic">
        </div>
        <button type="submit" class="btn">Save MQTT Settings</button>
      </form>
    </div>
  </div>
  <script>
    fetch('/api/config').then(r => r.json()).then(data => {
      if (data.wifi && data.wifi.ssid) document.getElementById('wifiSSID').value = data.wifi.ssid;
      if (data.mqtt) {
        document.getElementById('mqttHost').value = data.mqtt.host || '';
        document.getElementById('mqttPort').value = data.mqtt.port || 1883;
        document.getElementById('mqttTopic').value = data.mqtt.topic || '';
      }
    });
    function scanWiFi() {
      fetch('/api/config/wifiscan').then(r => r.json()).then(data => {
        const list = document.getElementById('wifiList');
        if (data.networks && data.networks.length > 0) {
          list.innerHTML = data.networks.map(n => '<div onclick="document.getElementById(\'wifiSSID\').value=\'' + n.ssid + '\'" style="padding:0.5rem;cursor:pointer;">' + n.ssid + ' (' + n.rssi + ' dBm)</div>').join('');
        }
      });
    }
    document.getElementById('wifiForm').addEventListener('submit', (e) => {
      e.preventDefault();
      fetch('/api/config/wifi', { method: 'PATCH', headers: {'Content-Type': 'application/json'}, body: JSON.stringify({wifi: {ssid: document.getElementById('wifiSSID').value, password: document.getElementById('wifiPassword').value}}) })
        .then(() => { document.getElementById('message').innerHTML = '<div class="success">WiFi settings saved! Restarting...</div>'; setTimeout(() => window.location.href = '/', 2000); });
    });
    document.getElementById('mqttForm').addEventListener('submit', (e) => {
      e.preventDefault();
      fetch('/api/config/mqtt', { method: 'PATCH', headers: {'Content-Type': 'application/json'}, body: JSON.stringify({mqtt: {host: document.getElementById('mqttHost').value, port: parseInt(document.getElementById('mqttPort').value), topic: document.getElementById('mqttTopic').value}}) })
        .then(() => { document.getElementById('message').innerHTML = '<div class="success">MQTT settings saved!</div>'; });
    });
  </script>
</body>
</html>)HTML";
  }
  
  if (path == "/system" || path == "/system.html") {
    return R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>System - OpenWatt P1 Reader</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #f5f5f5; color: #333; }
    .container { max-width: 800px; margin: 0 auto; padding: 20px; }
    header { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 2rem; border-radius: 10px; margin-bottom: 2rem; }
    nav { background: white; padding: 1rem; border-radius: 10px; margin-bottom: 2rem; }
    nav a { display: inline-block; padding: 0.5rem 1rem; margin-right: 1rem; text-decoration: none; color: #667eea; border-radius: 5px; }
    nav a:hover, nav a.active { background: #667eea; color: white; }
    .card { background: white; padding: 1.5rem; border-radius: 10px; margin-bottom: 1.5rem; }
    .info-item { display: flex; justify-content: space-between; padding: 0.75rem 0; border-bottom: 1px solid #eee; }
    .btn { padding: 0.75rem 1.5rem; background: #667eea; color: white; border: none; border-radius: 5px; cursor: pointer; margin-right: 1rem; }
    .btn-danger { background: #ef4444; }
  </style>
</head>
<body>
  <div class="container">
    <header><h1>🔧 System</h1></header>
    <nav>
      <a href="/">Dashboard</a>
      <a href="/live">Live Data</a>
      <a href="/settings">Settings</a>
      <a href="/system" class="active">System</a>
    </nav>
    <div class="card">
      <h2>System Information</h2>
      <div id="systemInfo">Loading...</div>
    </div>
    <div class="card">
      <h2>System Actions</h2>
      <button class="btn" onclick="reboot()">Reboot Device</button>
      <button class="btn btn-danger" onclick="factoryReset()">Factory Reset</button>
    </div>
  </div>
  <script>
    fetch('/api/system').then(r => r.json()).then(data => {
      document.getElementById('systemInfo').innerHTML = 
        '<div class="info-item"><span>Firmware Version:</span><span>' + data.firmware_version + '</span></div>' +
        '<div class="info-item"><span>Device ID:</span><span>' + data.device_id + '</span></div>' +
        '<div class="info-item"><span>Serial Number:</span><span>' + data.serial_number + '</span></div>';
    });
    function reboot() { if (confirm('Reboot device?')) fetch('/api/system/reboot', {method: 'PATCH'}); }
    function factoryReset() { if (confirm('WARNING: Erase all settings?')) fetch('/api/system/factory_reset', {method: 'PATCH'}); }
  </script>
</body>
</html>)HTML";
  }
  
  return ""; // 404
}

#endif

