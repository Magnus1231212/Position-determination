#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "config.h"

extern void initEspNow();

#ifdef ROLE_SLAVE
extern void startSniffer();
#endif

#ifdef ROLE_MASTER
extern void connectWiFi();
extern void syncTime();
extern void connectMQTT();
extern void mqttLoop(); // ← denne manglede måske
#endif

void setup()
{
  // ... samme som før
}

void loop()
{
#ifdef ROLE_MASTER
  mqttLoop();
#endif
  delay(10);
}