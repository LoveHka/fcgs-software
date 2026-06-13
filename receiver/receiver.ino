#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "telemetry_protocol.h"

RF24 radio(9, 10);
const byte address[6] = "Node1";

#define MAX_PAYLOAD_SIZE 32
#define HEADER_SIZE 1
#define CHUNK_DATA_SIZE (MAX_PAYLOAD_SIZE - HEADER_SIZE)
#define TOTAL_CHUNKS 4

struct ChunkPayload {
    uint8_t chunk_id;
    uint8_t data[CHUNK_DATA_SIZE];
};

#define CMD_CHUNK_MAGIC 0xAA

struct MagCalibChunk {
    uint8_t magic;
    uint8_t chunk_id;
    float data[6];
};

bool send_mag_calib_pending = false;
bool sent_chunk0 = false;
float mag_calib_full[12];
MagCalibChunk ack_payload;

void trigger_mag_calib_send(float* full_calib) {
    memcpy(mag_calib_full, full_calib, sizeof(mag_calib_full));
    send_mag_calib_pending = true;
    sent_chunk0 = false;
}

uint8_t rx_buffer[sizeof(DataPacket)];
uint8_t expected_chunk = 0;
unsigned long last_chunk_time = 0;
const unsigned long CHUNK_TIMEOUT = 100;

static uint8_t cmd_state = 0;
static uint16_t cmd_len = 0;
static uint8_t cmd_type = 0;
static uint8_t cmd_payload[128];
static uint16_t cmd_idx = 0;

void processSerialCommands() {
    while (Serial.available()) {
        uint8_t b = Serial.read();
        switch (cmd_state) {
            case 0: 
                if (b == TEL_SOF1) cmd_state = 1; 
                break;
            case 1: 
                cmd_state = (b == TEL_SOF2) ? 2 : 0; 
                break;
            case 2: 
                cmd_state = 3; 
                break;
            case 3: 
                cmd_type = b; 
                cmd_state = 4; 
                break;
            case 4: 
                cmd_len = b; 
                cmd_state = 5; 
                break;
            case 5:
                cmd_len |= (uint16_t)b << 8;
                cmd_idx = 0;
                cmd_state = 6;
                break;
            case 6:
                cmd_payload[cmd_idx++] = b;
                if (cmd_idx >= cmd_len) cmd_state = 7;
                break;
            case 7:
                cmd_state = 0;
                // ИСПРАВЛЕНО: правильная обработка CommandPacket
                if (cmd_type == TEL_TYPE_CMD && cmd_len >= sizeof(CommandPacket)) {
                    CommandPacket cmd;
                    memcpy(&cmd, cmd_payload, sizeof(cmd));
                    if (cmd.cmd == CMD_SET_MAG_CALIB) {
                        trigger_mag_calib_send(cmd.params);
                    }
                }
                break;
        }
    }
}

void setup() {
    Serial.begin(115200);
    
    radio.begin();
    radio.openReadingPipe(0, address);
    radio.setPALevel(RF24_PA_MIN);
    radio.setDataRate(RF24_2MBPS);
    radio.enableDynamicPayloads();
    radio.enableAckPayload();
    radio.startListening();
}

void loop() {
    processSerialCommands();

    if (radio.available()) {
        ChunkPayload chunk;
        radio.read(&chunk, sizeof(ChunkPayload));

        if (chunk.chunk_id == 0 || (millis() - last_chunk_time > CHUNK_TIMEOUT)) {
            expected_chunk = 0;
        }

        if (chunk.chunk_id == expected_chunk) {
            int bytes_to_copy = (chunk.chunk_id == TOTAL_CHUNKS - 1) ?
                                (sizeof(DataPacket) - (TOTAL_CHUNKS - 1) * CHUNK_DATA_SIZE) :
                                CHUNK_DATA_SIZE;

            memcpy(rx_buffer + (chunk.chunk_id * CHUNK_DATA_SIZE), chunk.data, bytes_to_copy);
            expected_chunk++;

            if (expected_chunk == TOTAL_CHUNKS) {
                DataPacket* received_packet = (DataPacket*)rx_buffer;
                
                Serial.write(TEL_SOF1);
                Serial.write(TEL_SOF2);
                Serial.write(TEL_VERSION);
                Serial.write(TEL_TYPE_DATA);

                writeU16LE(sizeof(DataPacket));
                Serial.write((const uint8_t*)received_packet, sizeof(DataPacket));

                uint16_t crc = crc16_ccitt((const uint8_t*)received_packet, sizeof(DataPacket));
                writeU16LE(crc);

                expected_chunk = 0;
            }
        } else {
            expected_chunk = 0;
        }

        last_chunk_time = millis();

        if (send_mag_calib_pending) {
            if (!sent_chunk0) {
                ack_payload.magic = CMD_CHUNK_MAGIC;
                ack_payload.chunk_id = 0;
                memcpy(ack_payload.data, mag_calib_full, 6 * sizeof(float));
                radio.writeAckPayload(0, &ack_payload, sizeof(ack_payload));
                sent_chunk0 = true;
            } else {
                ack_payload.magic = CMD_CHUNK_MAGIC;
                ack_payload.chunk_id = 1;
                memcpy(ack_payload.data, mag_calib_full + 6, 6 * sizeof(float));
                radio.writeAckPayload(0, &ack_payload, sizeof(ack_payload));
                sent_chunk0 = false;
                send_mag_calib_pending = false;
            }
        }
    }
}

static void writeU16LE(uint16_t v) {
    Serial.write((uint8_t)(v & 0xFF));
    Serial.write((uint8_t)(v >> 8));
}
