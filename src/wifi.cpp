#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

#ifdef ROLE_MASTER

void connectWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("[WiFi] Forbinder");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30)
    {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.printf(" OK: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("Kanal: %d\n", WiFi.channel());
    }
    else
    {
        Serial.println(" Fejl – genstarter");
        ESP.restart();
    }
}

#endif