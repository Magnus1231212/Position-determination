#include <Arduino.h>
#include "config.h"

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