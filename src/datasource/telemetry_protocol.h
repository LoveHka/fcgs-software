// datasource/telemetry_protocol.h
#pragma once

#include <cstdint>
#include <stddef.h>
#include <stdint.h>

enum : uint8_t {
  TEL_SOF1 = 0xAA,
  TEL_SOF2 = 0x55,
  TEL_VERSION = 1,
  TEL_TYPE_DATA = 1,
  TEL_TYPE_CMD = 2,
  TEL_TYPE_MESSAGE = 3
};

constexpr uint8_t MAX_MSG_LEN = 64;

enum : uint8_t { CMD_SET_MODE = 1, CMD_SET_MAG_CALIB = 2 };

enum : uint8_t { MODE_NORMAL = 0, MODE_MAG_CALIB = 1 };

// Your current packet, packed so no padding is inserted.
#pragma pack(push, 1)
struct DataPacket {
  float time; // time from MCU
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
  float Voltage;
  float d;
  float Tcycle;
};

struct CommandPacket {
  uint8_t cmd;
  float params[12]; // 3 смещения + 9 цифр с матрицы для магнитометра
};
#pragma pack(pop)

inline uint16_t crc16_ccitt(const uint8_t *data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; ++i) {
    crc ^= (uint16_t)data[i] << 8;
    for (int b = 0; b < 8; ++b) {
      if (crc & 0x8000)
        crc = (crc << 1) ^ 0x1021;
      else
        crc <<= 1;
    }
  }
  return crc;
}
