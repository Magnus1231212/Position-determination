#include <Arduino.h>
#include <WiFiClientSecure.h>
#include "config.h"

#ifdef ROLE_MASTER

#include <PubSubClient.h>

extern String getTimestamp();

static WiFiClientSecure secureClient;
static PubSubClient mqttClient(secureClient);

void connectMQTT()
{
    secureClient.setCACert(CA_CERT);

    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.setKeepAlive(30);

    int attempts = 0;
    while (!mqttClient.connected() && attempts < 5)
    {
        Serial.print("[MQTT] Forbinder...");
        if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS))
        {
            Serial.println(" OK");
        }
        else
        {
            Serial.printf(" Fejl: %d\n", mqttClient.state());
            delay(2000);
            attempts++;
        }
    }
}

void mqttPublish(const char *macHash, float x, float y)
{
    if (!mqttClient.connected())
        connectMQTT();

    char payload[200];
    snprintf(payload, sizeof(payload),
             "{\"id\":\"%s\",\"x\":%.1f,\"y\":%.1f,\"ts\":\"%s\"}",
             macHash, x, y, getTimestamp().c_str());

    mqttClient.publish(MQTT_TOPIC, payload);
    Serial.printf("[MQTT] Sendt: %s\n", payload);
    mqttClient.loop(); // Giv PubSubClient tid til at sende
    delay(100);
}

#endif