#include <Arduino.h>
#include "esp_wifi.h"
#include "config.h"
#include "shared_types.h"

extern float rssiToDistance(int rssi);
extern String hashMac(const uint8_t *mac);

#ifdef ROLE_SLAVE
extern void espnowSend(const EspNowReading &r);

void IRAM_ATTR snifferCallback(void *buf, wifi_promiscuous_pkt_type_t type)
{
    if (type != WIFI_PKT_MGMT)
        return;

    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    if ((pkt->payload[0] & 0xFC) != 0x40)
        return;

    static unsigned long lastSend = 0;
    unsigned long now = millis();
    if (now - lastSend < 500)
        return;
    lastSend = now;

    EspNowReading r;
    strncpy(r.device_id, MQTT_CLIENT_ID, sizeof(r.device_id));

    String h = hashMac(&pkt->payload[10]);
    strncpy(r.mac_hash, h.c_str(), sizeof(r.mac_hash));

    r.rssi = pkt->rx_ctrl.rssi;
    r.dist = rssiToDistance(r.rssi);
    r.x = MY_X;
    r.y = MY_Y;

    espnowSend(r);

    Serial.printf("[Sniffer] %s  RSSI=%d  dist=%.0fcm\n",
                  r.mac_hash, r.rssi, r.dist);
}

void startSniffer()
{
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&snifferCallback);
    esp_wifi_set_channel(2, WIFI_SECOND_CHAN_NONE);
    Serial.printf("[Sniffer] Kører på kanal 2 — pos: (%.0f, %.0f)\n",
                  (float)MY_X, (float)MY_Y);
}
#endif

#ifdef ROLE_MASTER
extern void updateStation(const char *id, float x, float y, float dist);

void IRAM_ATTR masterSnifferCallback(void *buf, wifi_promiscuous_pkt_type_t type)
{
    if (type != WIFI_PKT_MGMT)
        return;

    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    if ((pkt->payload[0] & 0xFC) != 0x40)
        return;

    static unsigned long lastSend = 0;
    unsigned long now = millis();
    if (now - lastSend < 2000)
        return;
    lastSend = now;

    String h = hashMac(&pkt->payload[10]);
    float dist = rssiToDistance(pkt->rx_ctrl.rssi);

    updateStation(MQTT_CLIENT_ID, MY_X, MY_Y, dist);

    Serial.printf("[Master Sniffer] %s  RSSI=%d  dist=%.0fcm\n",
                  h.c_str(), pkt->rx_ctrl.rssi, dist);
}

void startMasterSniffer()
{
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&masterSnifferCallback);
    Serial.printf("[Master Sniffer] Kører — pos: (%.0f, %.0f)\n",
                  (float)MY_X, (float)MY_Y);
}
#endif