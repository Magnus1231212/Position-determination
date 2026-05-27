#include <Arduino.h>
#include <math.h>
#include "config.h"
#include "shared_types.h"

#define MAX_SLAVES 8
#define READING_TTL 10000

struct StationReading
{
    float x, y, dist;
    unsigned long ts;
};

struct
{
    char id[12];
    StationReading r;
} slaves[MAX_SLAVES];

int slaveCount = 0;

float rssiToDistance(int rssi)
{
    const int txPower = -59;
    const float n = 3.0f;
    return pow(10.0f, (txPower - rssi) / (10.0f * n)) * 100.0f;
}

String hashMac(const uint8_t *mac)
{
    uint32_t h = 5381;
    for (int i = 0; i < 6; i++)
        h = h * 33 + mac[i];
    return String(h, HEX);
}

static StationReading *getSlot(const char *id)
{
    for (int i = 0; i < slaveCount; i++)
        if (strcmp(slaves[i].id, id) == 0)
            return &slaves[i].r;
    if (slaveCount >= MAX_SLAVES)
        return nullptr;
    strncpy(slaves[slaveCount].id, id, 12);
    return &slaves[slaveCount++].r;
}

void updateStation(const char *id, float x, float y, float dist)
{
    StationReading *slot = getSlot(id);
    if (!slot)
        return;
    slot->x = x;
    slot->y = y;
    slot->dist = dist;
    slot->ts = millis();
}

bool trilaterate(float &outX, float &outY)
{
    float ax[MAX_SLAVES], ay[MAX_SLAVES], ad[MAX_SLAVES];
    int valid = 0;
    unsigned long now = millis();

    for (int i = 0; i < slaveCount; i++)
    {
        if (now - slaves[i].r.ts < READING_TTL)
        {
            ax[valid] = slaves[i].r.x;
            ay[valid] = slaves[i].r.y;
            ad[valid] = slaves[i].r.dist;
            valid++;
        }
    }

    if (valid < 3)
    {
        Serial.printf("[Trilat] Kun %d gyldige stationer\n", valid);
        return false;
    }

    float A00 = 0, A01 = 0, A11 = 0, b0 = 0, b1 = 0;
    for (int i = 1; i < valid; i++)
    {
        float ai0 = 2.0f * (ax[i] - ax[0]);
        float ai1 = 2.0f * (ay[i] - ay[0]);
        float bi = ad[0] * ad[0] - ad[i] * ad[i] - ax[0] * ax[0] + ax[i] * ax[i] - ay[0] * ay[0] + ay[i] * ay[i];
        A00 += ai0 * ai0;
        A01 += ai0 * ai1;
        A11 += ai1 * ai1;
        b0 += ai0 * bi;
        b1 += ai1 * bi;
    }

    float det = A00 * A11 - A01 * A01;
    if (fabs(det) < 0.001f)
    {
        Serial.println("[Trilat] Singulær matrix");
        return false;
    }

    outX = (A11 * b0 - A01 * b1) / det;
    outY = (A00 * b1 - A01 * b0) / det;
    return true;
}