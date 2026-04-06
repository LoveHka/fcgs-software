// datasource/telemetry_protocol.h
#pragma once

#include <stdint.h>
#include <stddef.h>

enum : uint8_t {
    TEL_SOF1 = 0xAA,
    TEL_SOF2 = 0x55,
    TEL_VERSION = 1,
    TEL_TYPE_DATA = 1
};

// Your current packet, packed so no padding is inserted.
#pragma pack(push, 1)
struct DataPacket
{
    float time;      // time from MCU
    float az, ay, ax;
    float vx, vy, vz;
    float sx, sy, sz;
    float mx, my, mz;
    float H;
    float gx, gy, gz;
    float angx, angy, angz;
    float P;
    float T;
    float BarAlt;
    float V;
    float d;
    float Tcycle;
};
#pragma pack(pop)

inline uint16_t crc16_ccitt(const uint8_t* data, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (int b = 0; b < 8; ++b) {
            if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
            else              crc <<= 1;
        }
    }
    return crc;
}
