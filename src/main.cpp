#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "esp_wifi.h"
#include "config.h"

// ─── Konstanter ──────────────────────────────────────────────────────────────
#define SNIFF_DURATION_MS 10000
#define MAX_ENTRIES 50

// ─── Datastruktur til en måling ──────────────────────────────────────────────
struct Measurement
{
  uint8_t mac[6];
  int rssiSum;
  int count;
};

// ─── Globale variabler ───────────────────────────────────────────────────────
Measurement measurements[MAX_ENTRIES];
int measurementCount = 0;

WiFiClientSecure secureClient;
PubSubClient mqttClient(secureClient);

// ─── Sniffer callback ────────────────────────────────────────────────────────
void wifi_sniffer_callback(void *buf, wifi_promiscuous_pkt_type_t type)
{
  wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
  uint8_t *mac = pkt->payload + 10;
  int rssi = pkt->rx_ctrl.rssi;

  // Tjek om MAC allerede er i listen
  for (int i = 0; i < measurementCount; i++)
  {
    if (memcmp(measurements[i].mac, mac, 6) == 0)
    {
      measurements[i].rssiSum += rssi;
      measurements[i].count++;
      return;
    }
  }

  // Ny MAC — tilføj hvis der er plads
  if (measurementCount >= MAX_ENTRIES)
    return;
  memcpy(measurements[measurementCount].mac, mac, 6);
  measurements[measurementCount].rssiSum = rssi;
  measurements[measurementCount].count = 1;
  measurementCount++;
}

// ─── Hent ISO timestamp ───────────────────────────────────────────────────────
String getTimestamp()
{
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo))
    return "ukendt-tid";
  char buf[30];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &timeInfo);
  return String(buf);
}

// ─── NTP tidssynkronisering ───────────────────────────────────────────────────
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

// ─── Start sniffing ──────────────────────────────────────────────────────────
void startSniffing()
{
  measurementCount = 0;

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_callback);

  Serial.println("Sniffing starter...");
  delay(SNIFF_DURATION_MS);

  esp_wifi_set_promiscuous(false);
  esp_wifi_stop();
  esp_wifi_deinit();

  Serial.printf("Sniffing slut – %d målinger\n", measurementCount);
}

// ─── WiFi forbindelse ────────────────────────────────────────────────────────
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
    Serial.println("\nWi-Fi fejlede!");
  }
}

// ─── MQTT forbindelse ────────────────────────────────────────────────────────
void connectMQTT()
{
  secureClient.setCACert(CA_CERT);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  int attempts = 0;
  while (!mqttClient.connected() && attempts < 5)
  {
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS))
    {
      Serial.println("MQTT forbundet!");
    }
    else
    {
      Serial.printf("MQTT fejl: %d\n", mqttClient.state());
      delay(2000);
      attempts++;
    }
  }
}

// ─── Send målinger via MQTT ──────────────────────────────────────────────────
void sendMeasurements()
{
  String timestamp = getTimestamp();

  for (int i = 0; i < measurementCount; i++)
  {
    uint8_t *mac = measurements[i].mac;
    int avgRssi = measurements[i].rssiSum / measurements[i].count;

    char payload[160];
    snprintf(payload, sizeof(payload),
             "{\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"rssi\":%d,\"x\":%d,\"y\":%d,\"time\":\"%s\"}",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
             avgRssi, MY_X, MY_Y, timestamp.c_str());

    mqttClient.publish(MQTT_TOPIC, payload);
    Serial.println(payload);
    delay(10);
  }
}

// ─── Setup og loop ───────────────────────────────────────────────────────────
void setup()
{
  Serial.begin(115200);
  connectWiFi();
  syncTime();
  WiFi.disconnect(true);
  delay(500);
}

void loop()
{
  // Fase 1: Snif
  startSniffing();

  // Fase 2: Forbind og send
  connectWiFi();
  connectMQTT();
  sendMeasurements();

  // Fase 3: Afbryd WiFi igen
  mqttClient.disconnect();
  WiFi.disconnect(true);
  delay(1000);
}