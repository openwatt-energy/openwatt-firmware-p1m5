#include "web_ui.h"

// Include WiFi header for status checking
#include <WiFi.h>

// Embedded minimal CSS for offline/AP mode (when no internet connectivity)
// This is a compact, self-contained stylesheet that works without external dependencies
String getOfflineCSS() {
  return "<style>"
    /* CSS Reset and Base */
    "*{box-sizing:border-box;margin:0;padding:0}"
    "body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,'Helvetica Neue',Arial,sans-serif;background:#f9fafb;color:#111827;line-height:1.5;min-height:100vh}"
    
    /* Layout - Minimal but functional */
    ".max-w-7xl{max-width:80rem;margin:0 auto;padding:0 1rem}"
    ".max-w-3xl{max-width:48rem;margin:0 auto;padding:0 1rem}"
    ".mx-auto{margin-left:auto;margin-right:auto}"
    ".px-4{padding-left:1rem;padding-right:1rem}"
    ".py-6{padding-top:1.5rem;padding-bottom:1.5rem}"
    ".p-4{padding:1rem}.p-5{padding:1.25rem}.p-6{padding:1.5rem}"
    ".mb-4{margin-bottom:1rem}.mb-6{margin-bottom:1.5rem}.mt-1{margin-top:.25rem}.mt-3{margin-top:.75rem}"
    ".space-y-1>*+*{margin-top:.25rem}.space-y-3>*+*{margin-top:.75rem}.space-y-4>*+*{margin-top:1rem}"
    ".gap-1{gap:.25rem}.gap-3{gap:.75rem}.gap-4{gap:1rem}"
    ".flex{display:flex}.flex-col{flex-direction:column}.items-center{align-items:center}.justify-between{justify-content:space-between}.justify-center{justify-content:center}"
    ".grid{display:grid}.grid-cols-1{grid-template-columns:repeat(1,minmax(0,1fr))}"
    ".h-10{height:2.5rem}.h-16{height:4rem}.w-10{width:2.5rem}"
    ".h-2\\.5{height:.625rem}.w-2\\.5{width:.625rem}"
    ".rounded{border-radius:.25rem}.rounded-lg{border-radius:.5rem}.rounded-xl{border-radius:.75rem}"
    ".rounded-full{border-radius:9999px}"
    "@media(min-width:640px){.sm\\:flex-row{flex-direction:row}.sm\\:grid-cols-2{grid-template-columns:repeat(2,minmax(0,1fr))}.sm\\:w-auto{width:auto}}"
    "@media(min-width:1024px){.lg\\:grid-cols-3{grid-template-columns:repeat(3,minmax(0,1fr))}.lg\\:grid-cols-4{grid-template-columns:repeat(4,minmax(0,1fr))}}"
    
    /* Colors */
    ".bg-white{background:#fff}.bg-gray-50{background:#f9fafb}.bg-gray-100{background:#f3f4f6}.bg-gray-900{background:#111827}"
    ".bg-blue-50{background:#eff6ff}.bg-blue-600{background:#2563eb}"
    ".bg-emerald-50{background:#ecfdf5}.bg-emerald-100{background:#d1fae5}"
    ".bg-red-50{background:#fef2f2}.bg-red-100{background:#fee2e2}.bg-red-500{background:#ef4444}.bg-red-600{background:#dc2626}"
    ".bg-purple-50{background:#faf5ff}.bg-orange-50{background:#fff7ed}"
    ".bg-gradient-to-br{background:linear-gradient(to bottom right,var(--tw-gradient-stops))}"
    ".from-blue-50{--tw-gradient-from:#eff6ff;--tw-gradient-stops:var(--tw-gradient-from),var(--tw-gradient-to,rgb(239 246 255 / 0))}"
    ".to-blue-100{--tw-gradient-to:#dbeafe}"
    ".from-green-50{--tw-gradient-from:#f0fdf4;--tw-gradient-stops:var(--tw-gradient-from),var(--tw-gradient-to,rgb(240 253 244 / 0))}"
    ".to-green-100{--tw-gradient-to:#dcfce7}"
    ".from-purple-50{--tw-gradient-from:#faf5ff;--tw-gradient-stops:var(--tw-gradient-from),var(--tw-gradient-to,rgb(250 245 255 / 0))}"
    ".to-purple-100{--tw-gradient-to:#f3e8ff}"
    ".from-orange-50{--tw-gradient-from:#fff7ed;--tw-gradient-stops:var(--tw-gradient-from),var(--tw-gradient-to,rgb(255 247 237 / 0))}"
    ".to-orange-100{--tw-gradient-to:#ffedd5}"
    
    /* Text Colors */
    ".text-white{color:#fff}.text-gray-400{color:#9ca3af}.text-gray-500{color:#6b7280}.text-gray-600{color:#4b5563}.text-gray-700{color:#374151}.text-gray-900{color:#111827}"
    ".text-blue-400{color:#60a5fa}.text-blue-600{color:#2563eb}.text-blue-700{color:#1d4ed8}.text-blue-900{color:#1e3a8a}"
    ".text-emerald-400{color:#34d399}.text-emerald-600{color:#059669}.text-emerald-700{color:#047857}.text-emerald-800{color:#065f46}"
    ".text-red-400{color:#f87171}.text-red-500{color:#ef4444}.text-red-600{color:#dc2626}.text-red-700{color:#b91c1c}.text-red-800{color:#991b1b}"
    ".text-purple-400{color:#c084fc}.text-purple-600{color:#9333ea}.text-purple-900{color:#581c87}"
    ".text-green-400{color:#4ade80}.text-green-600{color:#16a34a}.text-green-900{color:#14532d}"
    ".text-orange-400{color:#fb923c}.text-orange-600{color:#ea580c}.text-orange-900{color:#7c2d12}"
    ".text-yellow-400{color:#facc15}.text-yellow-600{color:#ca8a04}"
    ".text-pink-400{color:#f472b6}.text-pink-600{color:#db2777}"
    ".text-cyan-400{color:#22d3ee}.text-teal-400{color:#2dd4bf}.text-amber-400{color:#fbbf24}"
    ".text-blue-500{color:#3b82f6}.text-emerald-500{color:#10b981}"
    
    /* Typography */
    ".font-mono{font-family:ui-monospace,SFMono-Regular,Menlo,Monaco,Consolas,monospace}"
    ".font-medium{font-weight:500}.font-semibold{font-weight:600}.font-bold{font-weight:700}"
    ".text-xs{font-size:.75rem}.text-sm{font-size:.875rem}.text-lg{font-size:1.125rem}.text-2xl{font-size:1.5rem}"
    ".text-center{text-align:center}"
    
    /* Borders */
    ".border{border-width:1px;border-style:solid}"
    ".border-b{border-bottom-width:1px}"
    ".border-gray-100{border-color:#f3f4f6}.border-gray-200{border-color:#e5e7eb}.border-gray-300{border-color:#d1d5db}.border-gray-700{border-color:#374151}.border-gray-800{border-color:#1f2937}"
    ".border-blue-200{border-color:#bfdbfe}.border-green-200{border-color:#bbf7d0}.border-purple-200{border-color:#e9d5ff}.border-orange-200{border-color:#fed7aa}"
    
    /* Components */
    ".shadow-sm{box-shadow:0 1px 2px 0 rgb(0 0 0 / .05)}.shadow-lg{box-shadow:0 10px 15px -3px rgb(0 0 0 / .1)}"
    ".sticky{position:sticky}.top-0{top:0}.z-50{z-index:50}"
    ".overflow-hidden{overflow:hidden}"
    ".cursor-pointer{cursor:pointer}"
    ".hover\\:bg-gray-50:hover{background:#f9fafb}.hover\\:bg-blue-100:hover{background:#dbeafe}.hover\\:bg-blue-700:hover{background:#1d4ed8}.hover\\:bg-red-700:hover{background:#b91c1c}"
    ".hover\\:underline:hover{text-decoration:underline}"
    ".transition{transition-property:all;transition-timing-function:cubic-bezier(.4,0,.2,1);transition-duration:.15s}"
    
    /* Forms */
    "input[type=text],input[type=password],input[type=number]{width:100%;padding:.5rem .75rem;border:1px solid #d1d5db;border-radius:.5rem;font-size:.875rem}"
    "input:focus{outline:none;border-color:#2563eb;box-shadow:0 0 0 3px rgba(37,99,235,.1)}"
    "label{display:block;font-size:.875rem;font-weight:500;color:#374151;margin-bottom:.25rem}"
    
    /* Buttons */
    "button{display:inline-flex;align-items:center;justify-content:center;gap:.5rem;padding:.5rem 1rem;border-radius:.5rem;font-size:.875rem;font-weight:500;cursor:pointer;border:none;transition:all .15s}"
    "button:hover{opacity:.9}"
    ".btn-primary{background:#2563eb;color:#fff}"
    ".btn-danger{background:#dc2626;color:#fff}"
    
    /* Custom utilities */
    ".inline-flex{display:inline-flex}.gap-2{gap:.5rem}.ml-2{margin-left:.5rem}.ml-auto{margin-left:auto}"
    ".px-2\\.5{padding-left:.625rem;padding-right:.625rem}.py-0\\.5{padding-top:.125rem;padding-bottom:.125rem}"
    ".py-1{padding-top:.25rem;padding-bottom:.25rem}.py-2{padding-top:.5rem;padding-bottom:.5rem}"
    ".px-3{padding-left:.75rem;padding-right:.75rem}.py-2{padding-top:.5rem;padding-bottom:.5rem}"
    ".pb-3{padding-bottom:.75rem}"
    ".inline-block{display:inline-block}"
    ".w-full{width:100%}"
    ".hidden{display:none}"
    
    /* Alerts/Messages */
    ".alert{padding:1rem;border-radius:.5rem;margin-bottom:1rem}"
    ".alert-success{background:#ecfdf5;color:#065f46}"
    ".alert-error{background:#fef2f2;color:#991b1b}"
    ".alert-info{background:#eff6ff;color:#1e40af}"
    
    /* Offline mode indicator */
    ".offline-mode{background:#fef3c7;border:1px solid #f59e0b;padding:0.75rem;border-radius:0.5rem;margin-bottom:1rem;color:#92400e}"
    "</style>";
}

// Online CSS - Loads Tailwind CSS from CDN for full styling when internet is available
String getOnlineCSS() {
  return "<link href=\"https://cdn.tailwindcss.com\" rel=\"stylesheet\">"
    "<link href=\"https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&display=swap\" rel=\"stylesheet\">"
    "<style>body{font-family:'Inter',system-ui,sans-serif}</style>";
}

// Detect if device is in online mode (WiFi connected to upstream network)
bool isOnlineMode() {
  // Check if STA mode is connected to an upstream WiFi network
  // Note: AP mode (192.168.4.1) is NOT "online" - it's just serving local AP
  return (WiFi.status() == WL_CONNECTED) && 
         (WiFi.getMode() & WIFI_STA) && 
         !(WiFi.getMode() & WIFI_AP);
}

// Get appropriate CSS based on connectivity mode
// ALWAYS use offline CSS for reliability - device should never depend on external CDN
String getThemeCSS() {
  // Always use offline CSS for reliability
  // Online mode detection can be unreliable and CDN may not be accessible
  return getOfflineCSS();
}

// Legacy function name for backward compatibility - redirects to getOfflineCSS
String getEmbeddedCSS() {
  return getOfflineCSS();
}

String getWebPage(const String& path) {
  // OpenWatt Logo - Embedded SVG (works offline)
  String logo = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 48 48\" class=\"h-10 w-10\">";
  logo += "<defs><linearGradient id=\"owGrad\" x1=\"0%\" y1=\"0%\" x2=\"100%\" y2=\"100%\"><stop offset=\"0%\" style=\"stop-color:#3b82f6;stop-opacity:1\" /><stop offset=\"100%\" style=\"stop-color:#1d4ed8;stop-opacity:1\" /></linearGradient></defs>";
  logo += "<circle cx=\"24\" cy=\"24\" r=\"22\" fill=\"url(#owGrad)\" />";
  logo += "<path fill=\"white\" d=\"M28 12l-12 16h8l-4 16 16-20h-8l4-12z\" />";
  logo += "</svg>";
  
  // Use theme CSS - always offline CSS for reliability
  String themeCSS = getThemeCSS();
  
  // Note: We always use offline CSS to ensure the UI works regardless of connectivity
  // This ensures the device web interface is always functional
  
  if (path == "/" || path == "/index.html") {
    String html = "<!DOCTYPE html><html lang=en><head>";
    html += "<meta charset=UTF-8><meta name=viewport content=\"width=device-width,initial-scale=1.0,maximum-scale=1.0,user-scalable=no\">";
    html += "<title>OpenWatt P1 Reader - Dashboard</title>";
    html += themeCSS;
    html += "</head><body>";
    html += "<nav class=\"bg-white border-b border-gray-200 sticky top-0 z-50\">";
    html += "<div class=\"max-w-7xl mx-auto px-4\"><div class=\"flex justify-between h-16\">";
    html += "<div class=\"flex items-center gap-3\"><div class=text-blue-600>" + logo + "</div></div>";
    html += "<div class=\"flex items-center gap-1\">";
    html += "<a href=/ class=\"px-3 py-2 rounded-lg text-sm font-medium bg-blue-50 text-blue-700\">Dashboard</a>";
    html += "<a href=/settings class=\"px-3 py-2 rounded-lg text-sm font-medium text-gray-600 hover:bg-gray-50\">Settings</a>";
    html += "<a href=/system class=\"px-3 py-2 rounded-lg text-sm font-medium text-gray-600 hover:bg-gray-50\">System</a>";
    html += "</div></div></div></nav>";
    html += "<main class=\"max-w-7xl mx-auto px-4 py-6\">";
    html += "<div class=mb-6><h1 class=\"text-2xl font-bold text-gray-900\">Dashboard</h1>";
    html += "<p class=\"mt-1 text-sm text-gray-500\" id=deviceInfo>Loading...</p></div>";
    // Connection Status Cards with WebSocket
    html += "<div class=\"grid grid-cols-1 sm:grid-cols-4 gap-4 mb-6\">";
    html += "<div class=\"bg-white rounded-xl shadow-sm border border-gray-200 p-5\">";
    html += "<div class=\"flex items-center gap-3\"><div class=\"w-10 h-10 rounded-lg bg-emerald-50 flex items-center justify-center\">";
    html += "<svg class=\"w-5 h-5 text-emerald-600\" fill=none stroke=currentColor viewBox=\"0 0 24 24\"><path stroke-linecap=round stroke-linejoin=round stroke-width=2 d=\"M8.111 16.404a5.5 5.5 0 017.778 0M12 20h.01m-7.08-7.071c3.904-3.905 10.236-3.905 14.141 0M1.394 9.393c5.857-5.857 15.355-5.857 21.213 0\"/></svg>";
    html += "</div><div><p class=\"text-sm font-medium text-gray-600\">WiFi</p>";
    html += "<span id=wifiStatus class=\"inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-medium bg-red-100 text-red-800\">Disconnected</span>";
    html += "</div></div></div>";
    html += "<div class=\"bg-white rounded-xl shadow-sm border border-gray-200 p-5\">";
    html += "<div class=\"flex items-center gap-3\"><div class=\"w-10 h-10 rounded-lg bg-blue-50 flex items-center justify-center\">";
    html += "<svg class=\"w-5 h-5 text-blue-600\" fill=none stroke=currentColor viewBox=\"0 0 24 24\"><path stroke-linecap=round stroke-linejoin=round stroke-width=2 d=\"M13 10V3L4 14h7v7l9-11h-7z\"/></svg>";
    html += "</div><div><p class=\"text-sm font-medium text-gray-600\">Meter</p>";
    html += "<span id=meterStatus class=\"inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-medium bg-red-100 text-red-800\">Disconnected</span>";
    html += "</div></div></div>";
    html += "<div class=\"bg-white rounded-xl shadow-sm border border-gray-200 p-5\">";
    html += "<div class=\"flex items-center gap-3\"><div class=\"w-10 h-10 rounded-lg bg-purple-50 flex items-center justify-center\">";
    html += "<svg class=\"w-5 h-5 text-purple-600\" fill=none stroke=currentColor viewBox=\"0 0 24 24\"><path stroke-linecap=round stroke-linejoin=round stroke-width=2 d=\"M3 15a4 4 0 004 4h9a5 5 0 10-.1-9.999 5.002 5.002 0 10-9.78 2.096A4.001 4.001 0 003 15z\"/></svg>";
    html += "</div><div><p class=\"text-sm font-medium text-gray-600\">MQTT</p>";
    html += "<span id=mqttStatus class=\"inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-medium bg-red-100 text-red-800\">Disconnected</span>";
    html += "</div></div></div>";
    html += "<div class=\"bg-white rounded-xl shadow-sm border border-gray-200 p-5\">";
    html += "<div class=\"flex items-center gap-3\"><div class=\"w-10 h-10 rounded-lg bg-orange-50 flex items-center justify-center\">";
    html += "<svg class=\"w-5 h-5 text-orange-600\" fill=none stroke=currentColor viewBox=\"0 0 24 24\"><path stroke-linecap=round stroke-linejoin=round stroke-width=2 d=\"M13 10V3L4 14h7v7l9-11h-7z\"/></svg>";
    html += "</div><div><p class=\"text-sm font-medium text-gray-600\">WebSocket</p>";
    html += "<span id=wsStatusBadge class=\"inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-medium bg-red-100 text-red-800\">Disconnected</span>";
    html += "</div></div></div></div>";
    // Quick Stats
    html += "<div class=\"bg-white rounded-xl shadow-sm border border-gray-200 p-6 mb-6\">";
    html += "<h2 class=\"text-lg font-semibold text-gray-900 mb-4\">Quick Stats</h2>";
    html += "<div class=\"grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4\">";
    html += "<div class=\"bg-gradient-to-br from-blue-50 to-blue-100 rounded-lg p-4 border border-blue-200\">";
    html += "<p class=\"text-sm font-medium text-blue-600 mb-1\">Power Import</p>";
    html += "<p class=\"text-2xl font-bold text-blue-900\" id=powerImport>--</p><p class=\"text-xs text-blue-600\">kW</p></div>";
    html += "<div class=\"bg-gradient-to-br from-green-50 to-green-100 rounded-lg p-4 border border-green-200\">";
    html += "<p class=\"text-sm font-medium text-green-600 mb-1\">Power Export</p>";
    html += "<p class=\"text-2xl font-bold text-green-900\" id=powerExport>--</p><p class=\"text-xs text-green-600\">kW</p></div>";
    html += "<div class=\"bg-gradient-to-br from-purple-50 to-purple-100 rounded-lg p-4 border border-purple-200\">";
    html += "<p class=\"text-sm font-medium text-purple-600 mb-1\">Total Consumed</p>";
    html += "<p class=\"text-2xl font-bold text-purple-900\" id=totalCons>--</p><p class=\"text-xs text-purple-600\">kWh</p></div>";
    html += "<div class=\"bg-gradient-to-br from-orange-50 to-orange-100 rounded-lg p-4 border border-orange-200\">";
    html += "<p class=\"text-sm font-medium text-orange-600 mb-1\">Total Produced</p>";
    html += "<p class=\"text-2xl font-bold text-orange-900\" id=totalProd>--</p><p class=\"text-xs text-orange-600\">kWh</p></div>";
    html += "</div></div>";
    // Real-time Meter Data via WebSocket
    html += "<div class=\"bg-gray-900 rounded-xl shadow-lg overflow-hidden\">";
    html += "<div class=\"p-4 border-b border-gray-800 flex items-center justify-between\">";
    html += "<h2 class=\"text-white font-semibold\">Real-time Meter Data</h2>";
    html += "<div class=\"flex items-center gap-2\">";
    html += "<span id=wsIndicator class=\"w-2.5 h-2.5 rounded-full bg-red-500\"></span>";
    html += "<span id=wsStatus class=\"text-sm text-gray-400\">Disconnected</span>";
    html += "</div></div>";
    html += "<div id=meterData class=\"p-4 font-mono text-sm space-y-1\"><div class=text-gray-500>Connecting to WebSocket...</div></div>";
    html += "</div>";
    // API Links Section
    html += "<div class=\"bg-white rounded-xl shadow-sm border border-gray-200 p-6 mt-6\">";
    html += "<h2 class=\"text-lg font-semibold text-gray-900 mb-4\">API Endpoints</h2>";
    html += "<div class=\"grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-3\">";
    html += "<a href=/api/system class=\"flex items-center p-3 bg-blue-50 rounded-lg hover:bg-blue-100 transition\"><span class=\"text-blue-600 font-mono text-sm\">GET</span><span class=\"ml-2 text-gray-700 text-sm\">/api/system</span></a>";
    html += "<a href=/api/state class=\"flex items-center p-3 bg-blue-50 rounded-lg hover:bg-blue-100 transition\"><span class=\"text-blue-600 font-mono text-sm\">GET</span><span class=\"ml-2 text-gray-700 text-sm\">/api/state</span></a>";
    html += "<a href=/api/config class=\"flex items-center p-3 bg-blue-50 rounded-lg hover:bg-blue-100 transition\"><span class=\"text-blue-600 font-mono text-sm\">GET</span><span class=\"ml-2 text-gray-700 text-sm\">/api/config</span></a>";
    html += "<a href=/api/meter class=\"flex items-center p-3 bg-blue-50 rounded-lg hover:bg-blue-100 transition\"><span class=\"text-blue-600 font-mono text-sm\">GET</span><span class=\"ml-2 text-gray-700 text-sm\">/api/meter</span></a>";
    html += "<a href=/api/meter/raw class=\"flex items-center p-3 bg-blue-50 rounded-lg hover:bg-blue-100 transition\"><span class=\"text-blue-600 font-mono text-sm\">GET</span><span class=\"ml-2 text-gray-700 text-sm\">/api/meter/raw</span></a>";
    html += "<a href=/api/v1/data class=\"flex items-center p-3 bg-blue-50 rounded-lg hover:bg-blue-100 transition\"><span class=\"text-blue-600 font-mono text-sm\">GET</span><span class=\"ml-2 text-gray-700 text-sm\">/api/v1/data</span></a>";
    html += "<a href=/api/config/wifiscan class=\"flex items-center p-3 bg-blue-50 rounded-lg hover:bg-blue-100 transition\"><span class=\"text-blue-600 font-mono text-sm\">GET</span><span class=\"ml-2 text-gray-700 text-sm\">/api/config/wifiscan</span></a>";
    html += "<a href=/api/debug/p1 class=\"flex items-center p-3 bg-blue-50 rounded-lg hover:bg-blue-100 transition\"><span class=\"text-blue-600 font-mono text-sm\">GET</span><span class=\"ml-2 text-gray-700 text-sm\">/api/debug/p1</span></a>";
    html += "<a href=/api/debug/nvs class=\"flex items-center p-3 bg-blue-50 rounded-lg hover:bg-blue-100 transition\"><span class=\"text-blue-600 font-mono text-sm\">GET</span><span class=\"ml-2 text-gray-700 text-sm\">/api/debug/nvs</span></a>";
    html += "<a href=/api/knock class=\"flex items-center p-3 bg-blue-50 rounded-lg hover:bg-blue-100 transition\"><span class=\"text-blue-600 font-mono text-sm\">GET</span><span class=\"ml-2 text-gray-700 text-sm\">/api/knock</span></a>";
    html += "</div></div>";
    html += "</div></main>";
    // JavaScript
    html += "<script>";
    html += "fetch('/api/system').then(r=>r.json()).then(d=>{document.getElementById('deviceInfo').textContent=d.firmware_version+' | '+d.device_id;});";
    html += "function updateStatus(){fetch('/api/state').then(r=>r.json()).then(data=>{const set=(id,ok)=>{const el=document.getElementById(id);el.textContent=ok?'Connected':'Disconnected';el.className='inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-medium '+(ok?'bg-emerald-100 text-emerald-800':'bg-red-100 text-red-800');};set('wifiStatus',data.wifi_connected);set('meterStatus',data.meter_connected);set('mqttStatus',data.cloud_connected);});}updateStatus();setInterval(updateStatus,5000);";
    html += "let ws=null;let reconnectTimeout=null;";
    html += "function hexToAscii(hex){if(!hex||hex==='N/A')return'N/A';let result='';for(let i=0;i<hex.length;i+=2){const byte=parseInt(hex.substr(i,2),16);if(byte>=32&&byte<127)result+=String.fromCharCode(byte);}return result||hex;}";
    html += "function connect(){const url=(window.location.protocol==='https:'?'wss:':'ws:')+'//'+window.location.host+'/api/live';ws=new WebSocket(url);";
    html += "ws.onopen=()=>{document.getElementById('wsIndicator').className='w-2.5 h-2.5 rounded-full bg-emerald-500';document.getElementById('wsStatus').textContent='Connected';document.getElementById('wsStatusBadge').textContent='Connected';document.getElementById('wsStatusBadge').className='inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-medium bg-emerald-100 text-emerald-800';};";
    html += "ws.onmessage=(e)=>{";
    html += "const data=JSON.parse(e.data);";
    html += "const meterId=hexToAscii(data['0-0:96.1.1']||'');";
    html += "const importKw=(data['1-0:1.7.0']||0);";
    html += "const exportKw=(data['1-0:2.7.0']||0);";
    html += "const cons1=(data['1-0:1.8.1']||0);";
    html += "const cons2=(data['1-0:1.8.2']||0);";
    html += "const prod1=(data['1-0:2.8.1']||0);";
    html += "const prod2=(data['1-0:2.8.2']||0);";
    html += "document.getElementById('powerImport').textContent=importKw.toFixed(3);";
    html += "document.getElementById('powerExport').textContent=exportKw.toFixed(3);";
    html += "document.getElementById('totalCons').textContent=(cons1+cons2).toFixed(1);";
    html += "document.getElementById('totalProd').textContent=(prod1+prod2).toFixed(1);";
    html += "let meterHtml='<div class=space-y-4>';";
    html += "meterHtml+='<div class=\"border-b border-gray-700 pb-3\"><h3 class=\"text-orange-400 font-semibold mb-2\">Identification</h3>';";
    html += "meterHtml+='<div class=\"flex justify-between text-gray-400 py-1\"><span>Meter ID (ASCII)</span><span class=text-emerald-400>'+meterId+'</span></div>';";
    html += "meterHtml+='<div class=\"flex justify-between text-gray-400 py-1\"><span>Equipment ID (Hex)</span><span class=text-blue-400>'+(data['0-0:96.1.1']||'N/A')+'</span></div>';";
    html += "meterHtml+='<div class=\"flex justify-between text-gray-400 py-1\"><span>Meter Model</span><span class=text-purple-400>'+(data['0-0:96.1.4']||'N/A')+'</span></div>';";
    html += "meterHtml+='<div class=\"flex justify-between text-gray-400 py-1\"><span>Timestamp</span><span class=text-yellow-400>'+(data['0-0:1.0.0']||'N/A')+'</span></div>';";
    html += "meterHtml+='<div class=\"flex justify-between text-gray-400 py-1\"><span>Tariff</span><span class=text-pink-400>'+(data['0-0:96.14.0']||'N/A')+'</span></div>';";
    html += "meterHtml+='</div>';";
    html += "meterHtml+='<div class=\"border-b border-gray-700 pb-3\"><h3 class=\"text-orange-400 font-semibold mb-2\">Power (kW)</h3>';";
    html += "meterHtml+='<div class=\"flex justify-between text-gray-400 py-1\"><span>Total Import</span><span class=text-yellow-400>'+importKw.toFixed(3)+' kW</span></div>';";
    html += "meterHtml+='<div class=\"flex justify-between text-gray-400 py-1\"><span>Total Export</span><span class=text-green-400>'+exportKw.toFixed(3)+' kW</span></div>';";
    html += "meterHtml+='<div class=\"flex justify-between text-gray-400 py-1\"><span>L1 Import</span><span class=text-blue-400>'+((data['1-0:21.7.0']||0)).toFixed(3)+' kW</span></div>';";
    html += "meterHtml+='<div class=\"flex justify-between text-gray-400 py-1\"><span>L2 Import</span><span class=text-blue-400>'+((data['1-0:41.7.0']||0)).toFixed(3)+' kW</span></div>';";
    html += "meterHtml+='<div class=\"flex justify-between text-gray-400 py-1\"><span>L3 Import</span><span class=text-blue-400>'+((data['1-0:61.7.0']||0)).toFixed(3)+' kW</span></div>';";
    html += "meterHtml+='</div>';";
    html += "meterHtml+='<div class=\"border-b border-gray-700 pb-3\"><h3 class=\"text-orange-400 font-semibold mb-2\">Energy (kWh)</h3>';";
    html += "meterHtml+='<div class=\"flex justify-between text-gray-400 py-1\"><span>Consumption T1</span><span class=text-purple-400>'+cons1.toFixed(3)+' kWh</span></div>';";
    html += "meterHtml+='<div class=\"flex justify-between text-gray-400 py-1\"><span>Consumption T2</span><span class=text-pink-400>'+cons2.toFixed(3)+' kWh</span></div>';";
    html += "meterHtml+='<div class=\"flex justify-between text-gray-400 py-1\"><span>Production T1</span><span class=text-cyan-400>'+prod1.toFixed(3)+' kWh</span></div>';";
    html += "meterHtml+='<div class=\"flex justify-between text-gray-400 py-1\"><span>Production T2</span><span class=text-teal-400>'+prod2.toFixed(3)+' kWh</span></div>';";
    html += "meterHtml+='</div>';";
    html += "meterHtml+='<div class=\"border-b border-gray-700 pb-3\"><h3 class=\"text-orange-400 font-semibold mb-2\">Voltage (V)</h3>';";
    html += "meterHtml+='<div class=\"flex justify-between text-gray-400 py-1\"><span>L1</span><span class=text-red-400>'+((data['1-0:32.7.0']||0)).toFixed(1)+' V</span></div>';";
    html += "meterHtml+='<div class=\"flex justify-between text-gray-400 py-1\"><span>L2</span><span class=text-red-400>'+((data['1-0:52.7.0']||0)).toFixed(1)+' V</span></div>';";
    html += "meterHtml+='<div class=\"flex justify-between text-gray-400 py-1\"><span>L3</span><span class=text-red-400>'+((data['1-0:72.7.0']||0)).toFixed(1)+' V</span></div>';";
    html += "meterHtml+='</div>';";
    html += "meterHtml+='<div><h3 class=\"text-orange-400 font-semibold mb-2\">Current (A)</h3>';";
    html += "meterHtml+='<div class=\"flex justify-between text-gray-400 py-1\"><span>L1</span><span class=text-amber-400>'+((data['1-0:31.7.0']||0)).toFixed(2)+' A</span></div>';";
    html += "meterHtml+='<div class=\"flex justify-between text-gray-400 py-1\"><span>L2</span><span class=text-amber-400>'+((data['1-0:51.7.0']||0)).toFixed(2)+' A</span></div>';";
    html += "meterHtml+='<div class=\"flex justify-between text-gray-400 py-1\"><span>L3</span><span class=text-amber-400>'+((data['1-0:71.7.0']||0)).toFixed(2)+' A</span></div>';";
    html += "meterHtml+='</div>';";
    html += "meterHtml+='</div>';";
    html += "document.getElementById('meterData').innerHTML=meterHtml;";
    html += "};";
    html += "ws.onclose=()=>{document.getElementById('wsIndicator').className='w-2.5 h-2.5 rounded-full bg-red-500';document.getElementById('wsStatus').textContent='Disconnected';document.getElementById('wsStatusBadge').textContent='Disconnected';document.getElementById('wsStatusBadge').className='inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-medium bg-red-100 text-red-800';reconnectTimeout=setTimeout(connect,3000);};";
    html += "};connect();";
    html += "window.addEventListener('beforeunload',()=>{if(ws)ws.close();if(reconnectTimeout)clearTimeout(reconnectTimeout);});";
    html += "</script></body></html>";
    return html;
  }
  
  if (path == "/live" || path == "/live.html") {
    String html = "<!DOCTYPE html><html lang=en><head>";
    html += "<meta charset=UTF-8><meta name=viewport content=\"width=device-width,initial-scale=1.0,maximum-scale=1.0,user-scalable=no\">";
    html += "<title>Live Data - OpenWatt</title>";
    html += themeCSS;
    html += "</head><body>";
    html += "<nav class=\"bg-white border-b border-gray-200 sticky top-0 z-50\">";
    html += "<div class=\"max-w-7xl mx-auto px-4\"><div class=\"flex justify-between h-16\">";
    html += "<div class=\"flex items-center gap-3\"><div class=text-blue-600>" + logo + "</div></div>";
    html += "<div class=\"flex items-center gap-1\">";
    html += "<a href=/ class=\"px-3 py-2 rounded-lg text-sm font-medium text-gray-600 hover:bg-gray-50\">Dashboard</a>";
    html += "<a href=/live class=\"px-3 py-2 rounded-lg text-sm font-medium bg-blue-50 text-blue-700\">Live</a>";
    html += "<a href=/settings class=\"px-3 py-2 rounded-lg text-sm font-medium text-gray-600 hover:bg-gray-50\">Settings</a>";
    html += "<a href=/system class=\"px-3 py-2 rounded-lg text-sm font-medium text-gray-600 hover:bg-gray-50\">System</a>";
    html += "</div></div></div></nav>";
    html += "<main class=\"max-w-7xl mx-auto px-4 py-6\">";
    html += "<div class=mb-6><h1 class=\"text-2xl font-bold text-gray-900\">Live Meter Data</h1>";
    html += "<p class=\"mt-1 text-sm text-gray-500\" id=liveVersion>Connecting...</p></div>";
    html += "<div class=\"bg-white rounded-xl shadow-sm border border-gray-200 p-4 mb-4\">";
    html += "<div class=\"flex items-center gap-2\">";
    html += "<span id=wsIndicator class=\"w-2.5 h-2.5 rounded-full bg-red-500\"></span>";
    html += "<span id=wsStatus class=\"text-sm font-medium text-gray-600\">Disconnected</span>";
    html += "</div></div>";
    html += "<div class=\"bg-gray-900 rounded-xl shadow-sm overflow-hidden\">";
    html += "<div class=\"p-4 border-b border-gray-800\"><h2 class=\"text-white font-semibold\">Real-time Measurements</h2></div>";
    html += "<div id=meterData class=\"p-4 font-mono text-sm space-y-2\"><div class=text-gray-500>Connecting to WebSocket...</div></div>";
    html += "</div></main>";
    html += "<script>fetch('/api/system').then(r=>r.json()).then(d=>{document.getElementById('liveVersion').textContent='FW '+d.firmware_version;});";
    html += "let ws=null;function connect(){const url=(window.location.protocol==='https:'?'wss:':'ws:')+'//'+window.location.host+'/api/live';ws=new WebSocket(url);";
    html += "ws.onopen=()=>{document.getElementById('wsIndicator').className='w-2.5 h-2.5 rounded-full bg-emerald-500';document.getElementById('wsStatus').textContent='Connected';};";
    html += "ws.onmessage=(e)=>{const data=JSON.parse(e.data);document.getElementById('meterData').innerHTML=";
    html += "'<div class=\"flex justify-between text-gray-400\"><span>Equipment ID</span><span class=text-emerald-400>'+(data['0-0:96.1.1']||'N/A')+'</span></div>'+";
    html += "'<div class=\"flex justify-between text-gray-400\"><span>Timestamp</span><span class=text-blue-400>'+(data['0-0:1.0.0']||'N/A')+'</span></div>'+";
    html += "'<div class=\"flex justify-between text-gray-400\"><span>Power Consumed</span><span class=text-yellow-400>'+(data['1-0:1.7.0']||0)+' kW</span></div>'+";
    html += "'<div class=\"flex justify-between text-gray-400\"><span>Power Produced</span><span class=text-green-400>'+(data['1-0:2.7.0']||0)+' kW</span></div>'+";
    html += "'<div class=\"flex justify-between text-gray-400\"><span>Consumption T1</span><span class=text-purple-400>'+(data['1-0:1.8.1']||0)+' kWh</span></div>'+";
    html += "'<div class=\"flex justify-between text-gray-400\"><span>Consumption T2</span><span class=text-pink-400>'+(data['1-0:1.8.2']||0)+' kWh</span></div>';};";
    html += "ws.onclose=()=>{document.getElementById('wsIndicator').className='w-2.5 h-2.5 rounded-full bg-red-500';document.getElementById('wsStatus').textContent='Disconnected - Reconnecting...';setTimeout(connect,3000);};}connect();</script>";
    html += "</body></html>";
    return html;
  }
  
  if (path == "/settings" || path == "/settings.html") {
    String html = "<!DOCTYPE html><html lang=en><head>";
    html += "<meta charset=UTF-8><meta name=viewport content=\"width=device-width,initial-scale=1.0,maximum-scale=1.0,user-scalable=no\">";
    html += "<title>Settings - OpenWatt</title>";
    html += themeCSS;
    html += "</head><body>";
    html += "<nav class=\"bg-white border-b border-gray-200 sticky top-0 z-50\">";
    html += "<div class=\"max-w-7xl mx-auto px-4\"><div class=\"flex justify-between h-16\">";
    html += "<div class=\"flex items-center gap-3\"><div class=text-blue-600>" + logo + "</div></div>";
    html += "<div class=\"flex items-center gap-1\">";
    html += "<a href=/ class=\"px-3 py-2 rounded-lg text-sm font-medium text-gray-600 hover:bg-gray-50\">Dashboard</a>";
    html += "<a href=/live class=\"px-3 py-2 rounded-lg text-sm font-medium text-gray-600 hover:bg-gray-50\">Live</a>";
    html += "<a href=/settings class=\"px-3 py-2 rounded-lg text-sm font-medium bg-blue-50 text-blue-700\">Settings</a>";
    html += "<a href=/system class=\"px-3 py-2 rounded-lg text-sm font-medium text-gray-600 hover:bg-gray-50\">System</a>";
    html += "</div></div></div></nav>";
    html += "<main class=\"max-w-3xl mx-auto px-4 py-6\">";
    html += "<div class=mb-6><h1 class=\"text-2xl font-bold text-gray-900\">Settings</h1>";
    html += "<p class=\"mt-1 text-sm text-gray-500\" id=settingsVersion>Loading...</p></div>";
    html += "<div id=message></div>";
    html += "<div class=\"bg-white rounded-xl shadow-sm border border-gray-200 p-6 mb-6\">";
    html += "<h2 class=\"text-lg font-semibold text-gray-900 mb-4\">WiFi Configuration</h2>";
    html += "<form id=wifiForm class=space-y-4>";
    html += "<div><button type=button onclick=scanWiFi() class=\"inline-flex items-center px-4 py-2 bg-blue-600 text-white rounded-lg hover:bg-blue-700 text-sm font-medium\">Scan Networks</button>";
    html += "<div id=wifiList class=\"mt-3 space-y-1\"></div></div>";
    html += "<div><label class=\"block text-sm font-medium text-gray-700 mb-1\">SSID</label>";
    html += "<input type=text id=wifiSSID required class=\"w-full px-3 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500\"></div>";
    html += "<div><label class=\"block text-sm font-medium text-gray-700 mb-1\">Password</label>";
    html += "<input type=password id=wifiPassword class=\"w-full px-3 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500\"></div>";
    html += "<button type=submit class=\"w-full sm:w-auto px-4 py-2 bg-blue-600 text-white rounded-lg hover:bg-blue-700 text-sm font-medium\">Save WiFi Settings</button>";
    html += "</form></div>";
    html += "<div class=\"bg-white rounded-xl shadow-sm border border-gray-200 p-6\">";
    html += "<h2 class=\"text-lg font-semibold text-gray-900 mb-4\">MQTT Configuration</h2>";
    html += "<form id=mqttForm class=space-y-4>";
    html += "<div><label class=\"block text-sm font-medium text-gray-700 mb-1\">Broker Host</label>";
    html += "<input type=text id=mqttHost class=\"w-full px-3 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500\"></div>";
    html += "<div><label class=\"block text-sm font-medium text-gray-700 mb-1\">Port</label>";
    html += "<input type=number id=mqttPort value=1883 class=\"w-full px-3 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500\"></div>";
    html += "<div><label class=\"block text-sm font-medium text-gray-700 mb-1\">Topic</label>";
    html += "<input type=text id=mqttTopic class=\"w-full px-3 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500\"></div>";
    html += "<button type=submit class=\"w-full sm:w-auto px-4 py-2 bg-blue-600 text-white rounded-lg hover:bg-blue-700 text-sm font-medium\">Save MQTT Settings</button>";
    html += "</form></div></main>";
    html += "<script>fetch('/api/system').then(r=>r.json()).then(d=>{document.getElementById('settingsVersion').textContent='FW '+d.firmware_version;});";
    html += "fetch('/api/config').then(r=>r.json()).then(data=>{if(data.wifi&&data.wifi.ssid)document.getElementById('wifiSSID').value=data.wifi.ssid;if(data.mqtt){document.getElementById('mqttHost').value=data.mqtt.host||'';document.getElementById('mqttPort').value=data.mqtt.port||1883;document.getElementById('mqttTopic').value=data.mqtt.topic||'';}});";
    html += "function getSignalIcon(rssi){if(rssi>=-50)return'<span class=text-green-600>▂▄▆█</span>';if(rssi>=-60)return'<span class=text-green-500>▂▄▆</span>';if(rssi>=-70)return'<span class=text-yellow-500>▂▄</span>';return'<span class=text-red-500>▂</span>';}";
    html += "function scanWiFi(){console.log('Scanning...');const list=document.getElementById('wifiList');list.innerHTML='<div class=text-sm text-gray-500>Scanning...</div>';fetch('/api/config/wifiscan').then(r=>{console.log('Response:',r.status);return r.json();}).then(data=>{console.log('Data:',data);if(data.error){list.innerHTML='<div class=text-sm text-red-600>'+data.error+'</div>';return;}const nets=data.networks||[];if(nets.length===0){list.innerHTML='<div class=text-sm text-gray-500>No networks found</div>';return;}list.innerHTML=nets.map(n=>'<div class=wifi-row flex items-center justify-between p-2 hover:bg-gray-50 cursor-pointer rounded text-sm data-ssid='+(n.ssid||'')+'><span class=font-medium>'+(n.ssid||'')+'</span><span class=text-gray-400>'+getSignalIcon(n.rssi)+'</span></div>').join('');list.querySelectorAll('.wifi-row').forEach(el=>{el.onclick=()=>{document.getElementById('wifiSSID').value=el.getAttribute('data-ssid')||'';list.innerHTML='';};});}).catch(err=>{console.error('Error:',err);list.innerHTML='<div class=text-sm text-red-600>Scan failed: '+err+'</div>';});}";
    html += "document.getElementById('wifiForm').addEventListener('submit',(e)=>{e.preventDefault();fetch('/api/config/wifi',{method:'PATCH',headers:{'Content-Type':'application/json'},body:JSON.stringify({wifi:{ssid:document.getElementById('wifiSSID').value,password:document.getElementById('wifiPassword').value}})}).then(()=>{document.getElementById('message').innerHTML='<div class=\"mb-4 p-4 bg-emerald-50 text-emerald-700 rounded-lg\">WiFi saved! Restarting...</div>';setTimeout(()=>window.location.href='/',2000);});});";
    html += "document.getElementById('mqttForm').addEventListener('submit',(e)=>{e.preventDefault();fetch('/api/config/mqtt',{method:'PATCH',headers:{'Content-Type':'application/json'},body:JSON.stringify({mqtt:{host:document.getElementById('mqttHost').value,port:parseInt(document.getElementById('mqttPort').value),topic:document.getElementById('mqttTopic').value}})}).then(()=>{document.getElementById('message').innerHTML='<div class=\"mb-4 p-4 bg-emerald-50 text-emerald-700 rounded-lg\">MQTT settings saved!</div>';});});</script>";
    html += "</body></html>";
    return html;
  }
  
  if (path == "/system" || path == "/system.html") {
    String html = "<!DOCTYPE html><html lang=en><head>";
    html += "<meta charset=UTF-8><meta name=viewport content=\"width=device-width,initial-scale=1.0,maximum-scale=1.0,user-scalable=no\">";
    html += "<title>System - OpenWatt</title>";
    html += themeCSS;
    html += "</head><body>";
    html += "<nav class=\"bg-white border-b border-gray-200 sticky top-0 z-50\">";
    html += "<div class=\"max-w-7xl mx-auto px-4\"><div class=\"flex justify-between h-16\">";
    html += "<div class=\"flex items-center gap-3\"><div class=text-blue-600>" + logo + "</div></div>";
    html += "<div class=\"flex items-center gap-1\">";
    html += "<a href=/ class=\"px-3 py-2 rounded-lg text-sm font-medium text-gray-600 hover:bg-gray-50\">Dashboard</a>";
    html += "<a href=/live class=\"px-3 py-2 rounded-lg text-sm font-medium text-gray-600 hover:bg-gray-50\">Live</a>";
    html += "<a href=/settings class=\"px-3 py-2 rounded-lg text-sm font-medium text-gray-600 hover:bg-gray-50\">Settings</a>";
    html += "<a href=/system class=\"px-3 py-2 rounded-lg text-sm font-medium bg-blue-50 text-blue-700\">System</a>";
    html += "</div></div></div></nav>";
    html += "<main class=\"max-w-3xl mx-auto px-4 py-6\">";
    html += "<div class=mb-6><h1 class=\"text-2xl font-bold text-gray-900\">System</h1>";
    html += "<p class=\"mt-1 text-sm text-gray-500\" id=systemVersion>Loading...</p></div>";
    html += "<div class=\"bg-white rounded-xl shadow-sm border border-gray-200 p-6 mb-6\">";
    html += "<h2 class=\"text-lg font-semibold text-gray-900 mb-4\">System Information</h2>";
    html += "<div id=systemInfo class=space-y-3><div class=\"flex justify-between py-2\"><span class=text-gray-600>Loading...</span></div></div>";
    html += "</div>";
    html += "<div class=\"bg-white rounded-xl shadow-sm border border-gray-200 p-6\">";
    html += "<h2 class=\"text-lg font-semibold text-gray-900 mb-4\">System Actions</h2>";
    html += "<div class=\"flex flex-col sm:flex-row gap-3\">";
    html += "<button onclick=reboot() class=\"inline-flex items-center justify-center px-4 py-2 bg-blue-600 text-white rounded-lg hover:bg-blue-700 text-sm font-medium\">Reboot Device</button>";
    html += "<button onclick=factoryReset() class=\"inline-flex items-center justify-center px-4 py-2 bg-red-600 text-white rounded-lg hover:bg-red-700 text-sm font-medium\">Factory Reset</button>";
    html += "</div></div></main>";
    html += "<script>fetch('/api/system').then(r=>r.json()).then(data=>{document.getElementById('systemVersion').textContent='FW '+data.firmware_version;document.getElementById('systemInfo').innerHTML='<div class=\"flex justify-between py-2 border-b border-gray-100\"><span class=text-gray-600>Firmware</span><span class=font-medium>'+data.firmware_version+'</span></div><div class=\"flex justify-between py-2 border-b border-gray-100\"><span class=text-gray-600>Device ID</span><span class=font-medium>'+data.device_id+'</span></div><div class=\"flex justify-between py-2 border-b border-gray-100\"><span class=text-gray-600>Serial</span><span class=font-medium>'+data.serial_number+'</span></div><div class=\"flex justify-between py-2\"><span class=text-gray-600>API URL</span><a href='+window.location.origin+'/api/system class=text-blue-600 hover:underline font-medium>'+window.location.origin+'</a></div>';});";
    html += "function reboot(){if(confirm('Reboot device?'))fetch('/api/system/reboot',{method:'PATCH'});}";
    html += "function factoryReset(){if(confirm('WARNING: This will erase all settings!'))fetch('/api/system/factory_reset',{method:'PATCH'});}</script>";
    html += "</body></html>";
    return html;
  }
  
  return "";
}
