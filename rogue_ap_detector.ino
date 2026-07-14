/*
 * Rogue AP / Evil-Twin Detector — ESP8266 / ESP32
 * -------------------------------------------------
 * Watches the air for access points broadcasting YOUR trusted SSID
 * from a BSSID (MAC) that isn't your real router, or with weaker
 * security than your real network — a classic "evil twin" attack.
 *
 * Purely passive: it only scans beacon frames, same as your phone's
 * WiFi picker. It never transmits deauth or spoofed frames.
 *
 * Configure TRUSTED_SSID / TRUSTED_BSSID / TRUSTED_ENC below, then
 * watch Serial for alerts. Wire an LED/buzzer to ALERT_PIN if you
 * want a physical alarm.
 */

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ESP32)
  #include <WiFi.h>
#endif

// ---- Config: fill in details of the network you're protecting ----
const char*   TRUSTED_SSID   = "HomeNetwork_5G";
const char*   TRUSTED_BSSID  = "AA:BB:CC:11:22:33"; // your router's real MAC, run wifi-scanner first to find it
const uint8_t TRUSTED_MIN_ENC_IS_WPA2 = true;        // alert if a twin offers weaker/no encryption

const unsigned long SCAN_INTERVAL_MS = 15000;
const int ALERT_PIN = LED_BUILTIN; // change to a buzzer/LED GPIO if desired

unsigned long lastScan = 0;

void setup() {
  Serial.begin(115200);
  delay(300);
  pinMode(ALERT_PIN, OUTPUT);
  digitalWrite(ALERT_PIN, HIGH); // LED_BUILTIN is usually active-LOW; HIGH = off
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.println("\nRogue AP Detector ready.");
  Serial.printf("Watching for impersonators of: \"%s\" (%s)\n\n", TRUSTED_SSID, TRUSTED_BSSID);
}

void loop() {
  if (millis() - lastScan >= SCAN_INTERVAL_MS) {
    runScan();
  }
}

void runScan() {
  lastScan = millis();
  int n = WiFi.scanNetworks();

  bool sawGenuine = false;
  bool sawImpostor = false;

  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i) != TRUSTED_SSID) continue;

    String bssid = WiFi.BSSIDstr(i);
    bool isEncrypted =
#if defined(ESP8266)
      WiFi.encryptionType(i) != ENC_TYPE_NONE;
#else
      WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
#endif

    bool bssidMatches = bssid.equalsIgnoreCase(TRUSTED_BSSID);
    bool weakened = TRUSTED_MIN_ENC_IS_WPA2 && !isEncrypted;

    if (bssidMatches && !weakened) {
      sawGenuine = true;
    } else {
      sawImpostor = true;
      Serial.println("!!! POSSIBLE EVIL TWIN DETECTED !!!");
      Serial.printf("    SSID:     %s\n", WiFi.SSID(i).c_str());
      Serial.printf("    BSSID:    %s  (expected %s)\n", bssid.c_str(), TRUSTED_BSSID);
      Serial.printf("    Channel:  %d\n", WiFi.channel(i));
      Serial.printf("    RSSI:     %d dBm\n", WiFi.RSSI(i));
      Serial.printf("    Open/weak encryption: %s\n", weakened ? "YES - red flag" : "no");
      Serial.println();
    }
  }

  if (sawImpostor) {
    alarm();
  } else {
    digitalWrite(ALERT_PIN, HIGH); // off
    Serial.printf("Scan clean. Genuine AP seen: %s\n\n", sawGenuine ? "yes" : "not in range");
  }

  WiFi.scanDelete();
}

void alarm() {
  // Simple blink pattern; swap for a buzzer tone() call if you wire one up.
  for (int i = 0; i < 6; i++) {
    digitalWrite(ALERT_PIN, LOW);
    delay(150);
    digitalWrite(ALERT_PIN, HIGH);
    delay(150);
  }
}
