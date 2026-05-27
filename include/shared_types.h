#pragma once

typedef struct
{
    char device_id[12];
    char mac_hash[9];
    int rssi;
    float dist;
    float x;
    float y;
} EspNowReading;