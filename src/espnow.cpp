#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "config.h"
#include "shared_types.h"

#ifdef ROLE_MASTER
extern void updateStation(const char *id, float x, float y, float dist);
extern bool trilaterate(float &outX, float &outY);
extern void mqttPublish(const char *macHash, float x, float y);

void onReceive(const uint8_t *mac, const uint8_t *data, int len)
{
    if (len != sizeof(EspNowReading))
        return;

    EspNowReading r;
    memcpy(&r, data, sizeof(r));

    updateStation(r.device_id, r.x, r.y, r.dist);

    Serial.printf("[ESP-NOW] Fra %s: RSSI=%d dist=%.0fcm\n",
                  r.device_id, r.rssi, r.dist);

    float px, py;
    if (!trilaterate(px, py))
        return;

    Serial.printf("[Trilat] Position: (%.1f, %.1f)\n", px, py);
    mqttPublish(r.mac_hash, px, py);
}
#endif

void initEspNow()
{
    esp_now_init();

#ifdef ROLE_MASTER
    esp_now_register_recv_cb(onReceive);
    Serial.println("[ESP-NOW] Master lytter");
    Serial.printf("[ESP-NOW] Master MAC: %s\n", WiFi.macAddress().c_str());
#endif

#ifdef ROLE_SLAVE
    uint8_t masterMac[] = MASTER_MAC;
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, masterMac, 6);
    peer.channel = 0;
    peer.encrypt = false;
    esp_now_add_peer(&peer);
    Serial.println("[ESP-NOW] Slave klar");
#endif
}

void espnowSend(const EspNowReading &r)
{
#ifdef ROLE_SLAVE
    uint8_t masterMac[] = MASTER_MAC;
    esp_now_send(masterMac, (uint8_t *)&r, sizeof(r));
#endif
}