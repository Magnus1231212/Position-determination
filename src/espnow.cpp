#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "config.h"
#include "shared_types.h"

// ======================================================
// IMPORTANT
// MUST MATCH YOUR ROUTER CHANNEL
// Example: if WiFi.channel() prints 11
// then use 11 here
// ======================================================

#define ESPNOW_CHANNEL 11

#ifdef ROLE_MASTER
extern void updateStation(const char *id, float x, float y, float dist);
extern bool trilaterate(float &outX, float &outY);
extern void mqttPublish(const char *macHash, float x, float y);
#endif

// ======================================================
// MASTER RECEIVE CALLBACK
// ======================================================

#ifdef ROLE_MASTER

void onReceive(const uint8_t *mac, const uint8_t *data, int len)
{
    if (len != sizeof(EspNowReading))
    {
        Serial.printf("[ESP-NOW] Invalid packet size: %d\n", len);
        return;
    }

    EspNowReading r;
    memcpy(&r, data, sizeof(r));

    char macStr[18];

    snprintf(macStr,
             sizeof(macStr),
             "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0],
             mac[1],
             mac[2],
             mac[3],
             mac[4],
             mac[5]);

    Serial.println();
    Serial.println("========== ESP-NOW RX ==========");
    Serial.printf("From     : %s\n", macStr);
    Serial.printf("Device   : %s\n", r.device_id);
    Serial.printf("RSSI     : %d\n", r.rssi);
    Serial.printf("Distance : %.2f cm\n", r.dist);
    Serial.printf("Coords   : (%.2f, %.2f)\n", r.x, r.y);

    updateStation(r.device_id, r.x, r.y, r.dist);

    float px, py;

    if (trilaterate(px, py))
    {
        Serial.println("======= TRILATERATION =======");
        Serial.printf("Position : (%.2f, %.2f)\n", px, py);

        mqttPublish(r.mac_hash, px, py);
    }
    else
    {
        Serial.println("[TRILAT] Waiting for more stations");
    }
}

#endif

// ======================================================
// SLAVE SEND CALLBACK
// ======================================================

#ifdef ROLE_SLAVE

void onSent(const uint8_t *mac, esp_now_send_status_t status)
{
    Serial.print("[ESP-NOW] Send status: ");

    if (status == ESP_NOW_SEND_SUCCESS)
        Serial.println("SUCCESS");
    else
        Serial.println("FAILED");
}

#endif

// ======================================================
// INIT
// ======================================================

void initEspNow()
{
    WiFi.mode(WIFI_STA);

#ifdef ROLE_SLAVE

    // SLAVES:
    // force onto same channel as router/master

    WiFi.disconnect();

    esp_wifi_set_channel(ESPNOW_CHANNEL,
                         WIFI_SECOND_CHAN_NONE);

#endif

    Serial.println();
    Serial.println("========== WIFI ==========");
    Serial.printf("MAC     : %s\n",
                  WiFi.macAddress().c_str());

    Serial.printf("Channel : %d\n",
                  WiFi.channel());

    // INIT ESP-NOW

    esp_err_t result = esp_now_init();

    if (result != ESP_OK)
    {
        Serial.printf("[ESP-NOW] Init FAILED: %d\n",
                      result);
        return;
    }

    Serial.println("[ESP-NOW] Init OK");

#ifdef ROLE_MASTER

    esp_now_register_recv_cb(onReceive);

    Serial.println("[ESP-NOW] MASTER READY");

#endif

#ifdef ROLE_SLAVE

    esp_now_register_send_cb(onSent);

    uint8_t masterMac[] = MASTER_MAC;

    Serial.printf("[ESP-NOW] Master MAC: "
                  "%02X:%02X:%02X:%02X:%02X:%02X\n",
                  masterMac[0],
                  masterMac[1],
                  masterMac[2],
                  masterMac[3],
                  masterMac[4],
                  masterMac[5]);

    esp_now_peer_info_t peer = {};

    memcpy(peer.peer_addr, masterMac, 6);

    peer.channel = ESPNOW_CHANNEL;

    peer.ifidx = WIFI_IF_STA;

    peer.encrypt = false;

    esp_err_t peerResult =
        esp_now_add_peer(&peer);

    if (peerResult != ESP_OK)
    {
        Serial.printf("[ESP-NOW] Add peer FAILED: %d\n",
                      peerResult);
        return;
    }

    Serial.println("[ESP-NOW] SLAVE READY");

#endif
}

// ======================================================
// SEND FUNCTION
// ======================================================

#ifdef ROLE_SLAVE

void espnowSend(const EspNowReading &r)
{
    uint8_t masterMac[] = MASTER_MAC;

    esp_err_t result =
        esp_now_send(masterMac,
                     (uint8_t *)&r,
                     sizeof(r));

    if (result == ESP_OK)
    {
        Serial.println("[ESP-NOW] Packet queued");
    }
    else
    {
        Serial.printf("[ESP-NOW] Send FAILED: %d\n",
                      result);
    }
}

#endif