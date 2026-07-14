/*
 * Captive Portal WiFi Manager — ESP8266
 * ---------------------------------------
 * Lets a headless IoT device get onto WiFi without hardcoding
 * credentials in the sketch. On first boot (or if saved WiFi fails),
 * it opens its own AP + captive portal; visiting it from a phone
 * shows a form to pick a network and enter its password. Credentials
 * are saved to flash (EEPROM) and used on subsequent boots.
 *
 * Libraries needed (Library Manager):
 *   - DNSServer (bundled with ESP8266 core)
 *   - ESP8266WebServer (bundled)
 *   - EEPROM (bundled)
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>

const byte DNS_PORT = 53;
const char* AP_SSID = "ESP-Setup";       // network the user connects to for setup
const char* AP_PASS = "";                // open network for easy access; add a password if you like
const unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000;

// EEPROM layout: [0]=magic byte, [1..33]=ssid (32 bytes), [34..98]=pass (64 bytes)
const int EEPROM_SIZE = 100;
const byte EEPROM_MAGIC = 0x42;

DNSServer dnsServer;
ESP8266WebServer server(80);

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);

  String savedSSID, savedPass;
  bool hasCreds = loadCredentials(savedSSID, savedPass);

  if (hasCreds && connectToWiFi(savedSSID, savedPass)) {
    Serial.println("Connected using saved credentials.");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    // ---- Your normal application code starts here ----
    return;
  }

  Serial.println("No valid saved WiFi — starting setup portal.");
  startCaptivePortal();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}

// ---------------- WiFi connect ----------------

bool connectToWiFi(const String& ssid, const String& pass) {
  if (ssid.length() == 0) return false;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  return WiFi.status() == WL_CONNECTED;
}

// ---------------- Captive portal ----------------

void startCaptivePortal() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, strlen(AP_PASS) ? AP_PASS : nullptr);

  IPAddress apIP = WiFi.softAPIP();
  dnsServer.start(DNS_PORT, "*", apIP); // redirect all DNS to us

  server.on("/", handleRoot);
  server.on("/scan", handleScan);
  server.on("/save", HTTP_POST, handleSave);
  server.onNotFound(handleRoot); // catch captive-portal probe requests

  server.begin();
  Serial.print("Setup portal live. Connect to AP \"");
  Serial.print(AP_SSID);
  Serial.print("\" then visit http://");
  Serial.println(apIP);
}

void handleRoot() {
  String html = R"HTML(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1">
<title>WiFi Setup</title>
<style>
  body{font-family:sans-serif;max-width:420px;margin:40px auto;padding:0 16px;}
  select,input,button{width:100%;padding:10px;margin:6px 0;box-sizing:border-box;font-size:16px;}
  button{background:#2563eb;color:#fff;border:none;border-radius:6px;padding:12px;}
</style></head><body>
<h2>Connect device to WiFi</h2>
<form action="/save" method="POST">
  <label>Network</label>
  <select name="ssid" id="ssid"><option>Scanning...</option></select>
  <label>Password</label>
  <input type="password" name="pass" placeholder="WiFi password">
  <button type="submit">Save &amp; Connect</button>
</form>
<script>
fetch('/scan').then(r=>r.json()).then(list=>{
  const sel = document.getElementById('ssid');
  sel.innerHTML = '';
  list.forEach(n=>{
    const opt = document.createElement('option');
    opt.value = n.ssid; opt.textContent = n.ssid + ' (' + n.rssi + ' dBm)';
    sel.appendChild(opt);
  });
});
</script>
</body></html>
)HTML";
  server.send(200, "text/html", html);
}

void handleScan() {
  int n = WiFi.scanNetworks();
  String json = "[";
  for (int i = 0; i < n; i++) {
    if (i) json += ",";
    json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleSave() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  saveCredentials(ssid, pass);

  server.send(200, "text/html",
    "<html><body style='font-family:sans-serif;text-align:center;margin-top:60px'>"
    "<h3>Saved. Rebooting and connecting to " + ssid + "...</h3></body></html>");

  delay(1500);
  ESP.restart();
}

// ---------------- EEPROM storage ----------------

bool loadCredentials(String& ssid, String& pass) {
  if (EEPROM.read(0) != EEPROM_MAGIC) return false;

  char ssidBuf[33] = {0};
  char passBuf[65] = {0};
  for (int i = 0; i < 32; i++) ssidBuf[i] = EEPROM.read(1 + i);
  for (int i = 0; i < 64; i++) passBuf[i] = EEPROM.read(34 + i);

  ssid = String(ssidBuf);
  pass = String(passBuf);
  return ssid.length() > 0;
}

void saveCredentials(const String& ssid, const String& pass) {
  EEPROM.write(0, EEPROM_MAGIC);
  for (int i = 0; i < 32; i++) EEPROM.write(1 + i, i < ssid.length() ? ssid[i] : 0);
  for (int i = 0; i < 64; i++) EEPROM.write(34 + i, i < pass.length() ? pass[i] : 0);
  EEPROM.commit();
}
