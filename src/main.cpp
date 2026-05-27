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
extern void mqttLoop();
#endif

void setup()
{
  Serial.begin(115200);
  delay(100);

#ifdef ROLE_MASTER
  connectWiFi();
  syncTime();
  connectMQTT();
#else
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
#endif

  initEspNow();

#ifdef ROLE_SLAVE
  startSniffer();
#endif

  Serial.println("[System] Klar");
}

void loop()
{
}