#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "config.h"
#include "esp_wifi.h"

extern void initEspNow();

#ifdef ROLE_SLAVE
extern void startSniffer();
#endif

#ifdef ROLE_MASTER
extern void connectWiFi();
extern void syncTime();
extern void connectMQTT();
extern void mqttLoop();
extern void startMasterSniffer();
#endif

void setup()
{
  Serial.begin(115200);
  delay(100);

#ifdef ROLE_MASTER
  connectWiFi();
  syncTime();
  connectMQTT();
  startMasterSniffer();
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