#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "esp_wifi.h"
#include "config.h"

#pragma region Global objects

// ─── Globalt wifi objekt ─────────────────────────────────────────────────────
WiFiClientSecure secureClient;
PubSubClient mqttClient(secureClient);

#pragma endregion
#pragma region Wi-Fi og MQTT setup

// ─────────────────────────────────────────────────────────────────────────────
//  Wifi
// ─────────────────────────────────────────────────────────────────────────────
void connectWiFi()
{
  Serial.printf("Forbinder til Wi-Fi: %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30)
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.printf("\nWi-Fi forbundet! IP: %s\n", WiFi.localIP().toString().c_str());
  }
  else
  {
    Serial.println("\nWi-Fi fejlede – går i deep sleep og prøver igen.");
    esp_deep_sleep(DEEP_SLEEP_DURATION_S * 1000000ULL);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  MQTT
// ─────────────────────────────────────────────────────────────────────────────
void connectMQTT()
{
  secureClient.setCACert(CA_CERT);
  // secureClient.setInsecure();
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setKeepAlive(30);

  Serial.printf("Forbinder til MQTT: %s:%d\n", MQTT_HOST, MQTT_PORT);

  int attempts = 0;
  while (!mqttClient.connected() && attempts < 5)
  {
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS))
    {
      Serial.println("MQTT forbundet!");
    }
    else
    {
      Serial.printf("MQTT fejl: %d – prøver igen om 2 sek.\n", mqttClient.state());
      delay(2000);
      attempts++;
    }
  }
}

#pragma endregion
#pragma region Time

// ─────────────────────────────────────────────────────────────────────────────
//  Hent ISO timestamp
// ─────────────────────────────────────────────────────────────────────────────
String getTimestamp()
{
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo))
    return "ukendt-tid";
  char buf[30];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &timeInfo);
  return String(buf);
}

// ─────────────────────────────────────────────────────────────────────────────
//  NTP tidssynkronisering
// ─────────────────────────────────────────────────────────────────────────────
void syncTime()
{
  configTzTime(TIMEZONE, NTP_SERVER);
  Serial.print("Venter på NTP sync");

  struct tm timeInfo;
  int attempts = 0;
  while (!getLocalTime(&timeInfo) && attempts < 20)
  {
    Serial.print(".");
    delay(500);
    attempts++;
  }

  if (attempts < 20)
  {
    char buf[30];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeInfo);
    Serial.printf("\nTid synkroniseret: %s\n", buf);
  }
  else
  {
    Serial.println("\nNTP sync fejlede – fortsætter alligevel.");
  }
}

#pragma endregion
#pragma region Setup og loop

void setup()
{
  Serial.begin(115200);
  connectWiFi();
  syncTime();
  connectMQTT();
}

void loop()
{
  if (!mqttClient.connected())
    connectMQTT();
  mqttClient.loop();
  delay(100);
}

#pragma endregion