#include <Arduino.h>
#include <WiFi.h>

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println();
    Serial.println("CR Field Analyzer - Board Bring-Up");
    Serial.println("----------------------------------");

    Serial.printf("Chip Model: %s\n", ESP.getChipModel());
    Serial.printf("Chip Revision: %d\n", ESP.getChipRevision());
    Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Flash Size: %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
    Serial.printf("PSRAM Size: %d MB\n", ESP.getPsramSize() / (1024 * 1024));

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(500);

    Serial.println("Starting Wi-Fi scan...");
    int networkCount = WiFi.scanNetworks();

    Serial.printf("Networks found: %d\n", networkCount);

    for (int i = 0; i < networkCount; i++) {
        Serial.printf(
            "%2d: %-32s RSSI: %4d dBm  CH: %2d\n",
            i + 1,
            WiFi.SSID(i).c_str(),
            WiFi.RSSI(i),
            WiFi.channel(i)
        );
    }

    Serial.println("Board bring-up test complete.");
}

void loop() {
    delay(1000);
}