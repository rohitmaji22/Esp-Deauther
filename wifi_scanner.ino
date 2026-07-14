/*
 * WiFi Scanner — ESP8266 / ESP32
 * -------------------------------
 * Passively scans for nearby WiFi networks and prints a sorted table
 * of SSID, RSSI, channel, and encryption type to Serial.
 *
 * No packets are transmitted at other devices; this only listens to
 * beacon frames, which is what every phone/laptop does when you open
 * its WiFi settings.
 *
 * Board: NodeMCU 1.0 (ESP-12E) or ESP32 dev board
 * Libraries: built-in (ESP8266WiFi.h or WiFi.h)
 */

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ESP32)
  #include <WiFi.h>
#endif

// ---- Config ----
const unsigned long SCAN_INTERVAL_MS = 10000;  // rescan every 10s
const bool SHOW_HIDDEN = true;

unsigned long lastScan = 0;

void setup() {
  Serial.begin(115200);
  delay(300);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.println("\nWiFi Scanner ready.\n");
  runScan();
}

void loop() {
  if (millis() - lastScan >= SCAN_INTERVAL_MS) {
    runScan();
  }
}

void runScan() {
  lastScan = millis();
  Serial.println("Scanning...");

  int n = WiFi.scanNetworks(false, SHOW_HIDDEN);

  if (n <= 0) {
    Serial.println("No networks found.\n");
    return;
  }

  // Build an index array and sort it by RSSI (descending) —
  // avoids reordering WiFi's internal scan result list.
  int order[n];
  for (int i = 0; i < n; i++) order[i] = i;
  for (int i = 0; i < n - 1; i++) {
    for (int j = i + 1; j < n; j++) {
      if (WiFi.RSSI(order[j]) > WiFi.RSSI(order[i])) {
        int tmp = order[i];
        order[i] = order[j];
        order[j] = tmp;
      }
    }
  }

  Serial.printf("%-32s %-6s %-4s %-5s %s\n", "SSID", "RSSI", "CH", "ENC", "BSSID");
  Serial.println("-------------------------------------------------------------------------");

  for (int i = 0; i < n; i++) {
    int idx = order[i];
    String ssid = WiFi.SSID(idx);
    if (ssid.length() == 0) ssid = "<hidden>";

    Serial.printf("%-32s %-6d %-4d %-5s %s\n",
                  ssid.c_str(),
                  WiFi.RSSI(idx),
                  WiFi.channel(idx),
                  encTypeStr(WiFi.encryptionType(idx)).c_str(),
                  WiFi.BSSIDstr(idx).c_str());
  }

  Serial.println();
  WiFi.scanDelete();
}

String encTypeStr(uint8_t encType) {
#if defined(ESP8266)
  switch (encType) {
    case ENC_TYPE_NONE: return "OPEN";
    case ENC_TYPE_WEP:  return "WEP";
    case ENC_TYPE_TKIP: return "WPA";
    case ENC_TYPE_CCMP: return "WPA2";
    case ENC_TYPE_AUTO: return "AUTO";
    default: return "?";
  }
#elif defined(ESP32)
  switch (encType) {
    case WIFI_AUTH_OPEN:            return "OPEN";
    case WIFI_AUTH_WEP:             return "WEP";
    case WIFI_AUTH_WPA_PSK:         return "WPA";
    case WIFI_AUTH_WPA2_PSK:        return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "ENT";
    case WIFI_AUTH_WPA3_PSK:        return "WPA3";
    default: return "?";
  }
#endif
}
