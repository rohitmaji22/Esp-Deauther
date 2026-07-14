# ESP8266/ESP32 Wireless Security Toolkit

A collection of NodeMCU-friendly projects for learning WiFi internals, network
monitoring, and defensive security — without building attack tooling.

## Projects

| Project | What it does | Board |
|---|---|---|
| [`wifi-scanner`](./wifi-scanner) | Passive scanner: lists nearby networks, RSSI, channel, encryption, sorted by signal strength | ESP8266 / ESP32 |
| [`rogue-ap-detector`](./rogue-ap-detector) | Watches for "evil twin" APs impersonating your home/office SSID and alerts you | ESP8266 / ESP32 |
| [`captive-portal-manager`](./captive-portal-manager) | Self-hosted WiFi provisioning portal (like WiFiManager) for headless IoT devices | ESP8266 / ESP32 |

All three only use standard scan/AP/station APIs — no deauth frames, no packet
injection, no promiscuous-mode sniffing of other people's traffic.

## Hardware

- NodeMCU 1.0 (ESP-12E) or any ESP8266 board, or an ESP32 dev board
- USB cable
- Optional: SSD1306 OLED (128x64, I2C) for on-device display, buzzer/LED for alerts

## Toolchain

Arduino IDE (Boards Manager → add `http://arduino.esp8266.com/stable/package_esp8266com_index.json`)
or PlatformIO (recommended — `platformio.ini` examples included in each folder).

## If you actually need to test your own network's resilience

Deauth/jamming tools are out of scope here, but legitimate, well-documented
options exist for testing **networks you own or have written authorization
to test**:

- **Aircrack-ng suite** — the standard, audited toolkit for 802.11 security
  assessment, run from a Linux machine with a monitor-mode-capable adapter.
- **Wireshark** — passive traffic analysis for diagnosing your own network.
- Many countries require explicit authorization before doing any 802.11
  disruption testing, even on your own gear, because signals aren't
  containable to your property line — check local regulations first.

## License

MIT — do whatever you like with this code, just don't point it at networks
you don't own or have permission to test.
