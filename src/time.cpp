#include <Arduino.h>
#include "config.h"

#ifdef ROLE_MASTER

String getTimestamp()
{
    struct tm timeInfo;
    if (!getLocalTime(&timeInfo))
        return "ukendt-tid";
    char buf[30];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &timeInfo);
    return String(buf);
}

void syncTime()
{
    configTzTime(TIMEZONE, NTP_SERVER);
    Serial.print("[NTP] Venter på sync");

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
        Serial.printf("\n[NTP] Synkroniseret: %s\n", buf);
    }
    else
    {
        Serial.println("\n[NTP] Fejlede – fortsætter alligevel.");
    }
}

#endif