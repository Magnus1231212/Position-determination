#pragma once

typedef struct
{
    char device_id[12];
    char mac_hash[12];
    int rssi;
    float dist;
    float x;
    float y;
} EspNowReading;