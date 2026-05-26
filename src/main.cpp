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
  int rssi;
};

// ─── Globale variabler ───────────────────────────────────────────────────────
Measurement measurements[MAX_ENTRIES];
int measurementCount = 0;

WiFiClientSecure secureClient;
PubSubClient mqttClient(secureClient);

// ─── Sniffer callback ────────────────────────────────────────────────────────
void wifi_sniffer_callback(void *buf, wifi_promiscuous_pkt_type_t type)
{
  if (measurementCount >= MAX_ENTRIES)
    return;

  wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
  uint8_t *mac = pkt->payload + 10;
  int rssi = pkt->rx_ctrl.rssi;

  // Gem målingen
  memcpy(measurements[measurementCount].mac, mac, 6);
  measurements[measurementCount].rssi = rssi;
  measurementCount++;
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
  for (int i = 0; i < measurementCount; i++)
  {
    uint8_t *mac = measurements[i].mac;
    int rssi = measurements[i].rssi;

    char payload[128];
    snprintf(payload, sizeof(payload),
             "{\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"rssi\":%d,\"x\":%d,\"y\":%d}",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
             rssi, MY_X, MY_Y);

    mqttClient.publish(MQTT_TOPIC, payload);
    Serial.println(payload);
    delay(10);
  }
}

// ─── Setup og loop ───────────────────────────────────────────────────────────
void setup()
{
  Serial.begin(115200);
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