#include <Arduino.h>
#include "esp_wifi.h"
#include "config.h"
#include "shared_types.h"

#ifdef ROLE_SLAVE

extern float rssiToDistance(int rssi);
extern String hashMac(const uint8_t *mac);
extern void espnowSend(const EspNowReading &r);

void IRAM_ATTR snifferCallback(void *buf, wifi_promiscuous_pkt_type_t type)
{
    if (type != WIFI_PKT_MGMT)
        return;

    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    if ((pkt->payload[0] & 0xFC) != 0x40)
        return;

    // Throttle – send max én gang hvert 500ms
    static unsigned long lastSend = 0;
    unsigned long now = millis();
    if (now - lastSend < 2000)
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
    esp_wifi_set_channel(11, WIFI_SECOND_CHAN_NONE);
    Serial.printf("[Sniffer] Kører på kanal 11 — pos: (%.0f, %.0f)\n",
                  (float)MY_X, (float)MY_Y);
}

#endif