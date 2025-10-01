#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

const char* apSSID = "ESP32_Config";
const char* apPass = ""; // AP abierto por simplicidad; si quieres añade password
const byte DNS_PORT = 53;

WebServer server(80);
DNSServer dnsServer;
Preferences preferences;

String savedSSID = "";
String savedPass = "";

bool shouldRestartAfterConnect = false;
unsigned long lastScanTime = 0;

void saveCredentials(const String &ssid, const String &pass) {
  preferences.begin("wifi", false);
  preferences.putString("ssid", ssid);
  preferences.putString("pass", pass);
  preferences.end();
  savedSSID = ssid;
  savedPass = pass;
  Serial.println("Credenciales guardadas.");
}

void clearCredentials() {
  preferences.begin("wifi", false);
  preferences.remove("ssid");
  preferences.remove("pass");
  preferences.end();
  savedSSID = "";
  savedPass = "";
  Serial.println("Credenciales borradas.");
}

void loadCredentials() {
  preferences.begin("wifi", true);
  savedSSID = preferences.getString("ssid", "");
  savedPass = preferences.getString("pass", "");
  preferences.end();
  if (savedSSID != "") {
    Serial.print("Credencial cargada: ");
    Serial.println(savedSSID);
  } else {
    Serial.println("No hay credenciales guardadas.");
  }
}

void startAPMode() {
  Serial.println("Iniciando AP modo de configuración...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID, apPass);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP: ");
  Serial.println(myIP);

  // Start DNS server to redirect all domains to the ESP IP (captive portal)
  dnsServer.start(DNS_PORT, "*", myIP);
}

bool connectToSavedWiFi(unsigned long timeoutMs = 10000) {
  if (savedSSID == "") return false;
  Serial.printf("Conectando a %s ...\n", savedSSID.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSSID.c_str(), savedPass.c_str());
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Conectado! IP: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("No se pudo conectar con credenciales guardadas.");
    return false;
  }
}

// -------- HTML/JS servidos por el ESP -----------
String indexPage() {
  // Página simple con JS para escanear SSID (/scan) y enviar credenciales (/connect)
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>Configuración WiFi ESP32</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, Helvetica, sans-serif; margin: 10px; padding: 0; text-align:center; }
    .card{ max-width: 420px; margin: auto; border-radius: 8px; box-shadow: 0 2px 8px rgba(0,0,0,0.2); padding: 16px; }
    button{ padding:10px 16px; margin:8px; font-size:16px; }
    input{ padding:8px; width: 90%; margin:6px 0; font-size:16px; }
    ul{ list-style:none; padding-left:0; max-height:200px; overflow:auto; text-align:left; }
    li{ padding:8px; border-bottom:1px solid #eee; cursor:pointer; }
    li:hover{ background:#f0f0f0; }
    .status { margin-top:10px; font-weight: bold; }
  </style>
</head>
<body>
  <div class="card">
    <h2>Configuración WiFi</h2>
    <div>
      <button onclick="doScan()">Escanear redes</button>
      <button onclick="forget()">Olvidar credenciales</button>
    </div>
    <div id="networks">
      <p>Pulse "Escanear redes" para listar SSID.</p>
    </div>

    <div id="form" style="display:none;">
      <p>SSID seleccionado: <b id="ssidSelected"></b></p>
      <input type="password" id="pwd" placeholder="Contraseña WiFi">
      <div>
        <button onclick="connect()">Conectar</button>
      </div>
    </div>

    <p class="status" id="status"></p>
  </div>

<script>
function mkElem(tag, txt){ let e=document.createElement(tag); e.innerText=txt; return e; }

function doScan(){
  document.getElementById('networks').innerHTML = '<p>Escaneando... (puede tardar 3-6s)</p>';
  fetch('/scan').then(r=>r.json()).then(list=>{
    let div=document.getElementById('networks');
    div.innerHTML='';
    if(list.length==0){ div.appendChild(mkElem('p','No se detectaron redes.')); return; }
    let ul=document.createElement('ul');
    list.forEach(ssid=>{
      let li=document.createElement('li');
      li.innerText = ssid;
      li.onclick = function(){ selectSSID(ssid); };
      ul.appendChild(li);
    });
    div.appendChild(ul);
  }).catch(e=>{
    document.getElementById('networks').innerHTML = '<p>Error en escaneo</p>';
  });
}

function selectSSID(s){
  document.getElementById('ssidSelected').innerText = s;
  document.getElementById('form').style.display = 'block';
  document.getElementById('status').innerText = '';
}

function connect(){
  let ssid = document.getElementById('ssidSelected').innerText;
  let pwd = document.getElementById('pwd').value;
  if(!ssid){ alert('Selecciona un SSID'); return; }
  document.getElementById('status').innerText = 'Intentando conectar...';
  fetch('/connect', {
    method:'POST',
    headers: {'Content-Type':'application/json'},
    body: JSON.stringify({ssid:ssid, pass:pwd})
  }).then(r=>r.json()).then(obj=>{
    if(obj.success){
      document.getElementById('status').innerText = 'Conectado: ' + obj.ip;
      // opcional: mostrar mensaje y recomendar desconectarse del AP para probar la red real
    } else {
      document.getElementById('status').innerText = 'Error: ' + obj.message;
    }
  }).catch(e=>{
    document.getElementById('status').innerText = 'Error en petición';
  });
}

function forget(){
  fetch('/forget').then(r=>r.text()).then(t=>{
    alert(t);
    location.reload();
  });
}

// Auto redirect to main page if already connected
fetch('/status').then(r=>r.json()).then(s=>{
  if(s.connected){
    document.getElementById('status').innerText = 'Ya conectado a: ' + s.ssid + ' ('+s.ip+')';
  }
});
</script>
</body>
</html>
)rawliteral";
  return html;
}

// -------------- Handler endpoints ----------------
void handleRoot() {
  server.send(200, "text/html", indexPage());
}

void handleScan() {
  // For simplicity we do a blocking scan here.
  Serial.println("Escaneando redes WiFi...");
  int n = WiFi.scanNetworks();
  Serial.printf("Encontradas %d redes\n", n);
  String json = "[";
  for (int i = 0; i < n; ++i) {
    String ssid = WiFi.SSID(i);
    ssid.replace("\"",""); // limpiar comillas si hay
    json += "\"" + ssid + "\"";
    if (i < n - 1) json += ",";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleConnect() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Usar POST con JSON {ssid, pass}");
    return;
  }
  String body = server.arg("plain");
  Serial.print("POST /connect body: ");
  Serial.println(body);

  // parse simple JSON manually (evita dependencia)
  String ssid = "";
  String pass = "";

  // simple parsing (no robust JSON lib)
  int s1 = body.indexOf("\"ssid\"");
  if (s1 >= 0) {
    int c = body.indexOf(':', s1);
    int q1 = body.indexOf('"', c+1);
    int q2 = body.indexOf('"', q1+1);
    if (q1>=0 && q2>q1) ssid = body.substring(q1+1, q2);
  }
  int p1 = body.indexOf("\"pass\"");
  if (p1 >= 0) {
    int c = body.indexOf(':', p1);
    int q1 = body.indexOf('"', c+1);
    int q2 = body.indexOf('"', q1+1);
    if (q1>=0 && q2>q1) pass = body.substring(q1+1, q2);
  }

  if (ssid.length() == 0) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"SSID vacío\"}");
    return;
  }

  // intentar conectar con las credenciales recibidas
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  unsigned long start = millis();
  bool connected = false;
  while (millis() - start < 10000) {
    if (WiFi.status() == WL_CONNECTED) { connected = true; break; }
    delay(200);
  }

  if (connected) {
    // guardar credenciales
    saveCredentials(ssid, pass);
    String ip = WiFi.localIP().toString();
    String resp = "{\"success\":true,\"ip\":\"" + ip + "\"}";
    server.send(200, "application/json", resp);
    Serial.println("Conexión exitosa desde /connect. IP: " + ip);
    // Opcional: apagar AP y DNS para que el dispositivo opere como STA
    dnsServer.stop();
    delay(200);
    WiFi.softAPdisconnect(true);
    shouldRestartAfterConnect = true; // se puede reiniciar si quieres, o simplemente seguir
  } else {
    // No conectado
    server.send(200, "application/json", "{\"success\":false,\"message\":\"No se pudo conectar con esas credenciales\"}");
  }
}

void handleStatus() {
  bool connected = (WiFi.status() == WL_CONNECTED);
  String ss = connected ? WiFi.SSID() : "";
  String ip = connected ? WiFi.localIP().toString() : "";
  String json = "{\"connected\":" + String(connected ? "true":"false") + ",\"ssid\":\"" + ss + "\",\"ip\":\"" + ip + "\"}";
  server.send(200, "application/json", json);
}

void handleForget() {
  clearCredentials();
  server.send(200, "text/plain", "Credenciales borradas. Reiniciando...");
  delay(2000);
  ESP.restart();
}

// -------------- Setup y loop ----------------------
void setup() {
  Serial.begin(115200);
  delay(200);

  loadCredentials();

  // Intenta conectarse si hay credenciales guardadas
  bool connected = false;
  if (savedSSID != "") {
    connected = connectToSavedWiFi(8000);
  }

  if (!connected) {
    // Si no pudo conectar o no hay credenciales, inicia AP con captive portal
    startAPMode();
  } else {
    // Si ya conectado, vamos al modo STA normal (server tambien puede servir si quieres)
    Serial.println("Modo STA operativo.");
  }

  // Rutas del servidor
  server.on("/", handleRoot);
  server.on("/scan", HTTP_GET, handleScan);
  server.on("/connect", HTTP_POST, handleConnect);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/forget", HTTP_GET, handleForget);
  // servir cualquier path con index (captura navegaciones)
  server.onNotFound([](){
    // si existe recurso: no usamos filesystem, así que devolvemos index para SPA
    handleRoot();
  });

  server.begin();
  Serial.println("Servidor HTTP iniciado.");
}

void loop() {
  // manejar DNS para captive portal
  dnsServer.processNextRequest();
  server.handleClient();

  // Si nos conectamos vía /connect y queremos reiniciar para asegurar estado limpio:
  if (shouldRestartAfterConnect) {
    Serial.println("Reiniciando para completar la conexión...");
    delay(1000);
    ESP.restart();
  }

  // Si estamos en STA y perdimos conexión, intentar reconectar
  static unsigned long lastReconnectAttempt = 0;
  if (WiFi.status() != WL_CONNECTED && savedSSID != "") {
    if (millis() - lastReconnectAttempt > 10000) {
      Serial.println("Intentando reconectar a WiFi guardada...");
      lastReconnectAttempt = millis();
      if (connectToSavedWiFi(8000)) {
        Serial.println("Reconexión exitosa.");
      } else {
        Serial.println("Fallo reconexión: vuelvo a AP para reconfiguración.");
        // volver a modo AP para reconfiguración
        startAPMode();
      }
    }
  }
  delay(2);
}