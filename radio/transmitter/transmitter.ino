//transmitter code
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "telemetry_protocol.h"

RF24 radio(9, 10);
const byte address[6] = "Node1";

// Параметры фрагментации
#define MAX_PAYLOAD_SIZE 32
#define HEADER_SIZE 1
#define CHUNK_DATA_SIZE (MAX_PAYLOAD_SIZE - HEADER_SIZE) // 31 байт данных
#define TOTAL_CHUNKS 4 // 104 байта / 31 байт = 3.35 -> 4 куска

struct ChunkPayload {
  uint8_t chunk_id; // Номер куска (0, 1, 2 или 3)
  uint8_t data[CHUNK_DATA_SIZE];
};

DataPacket packet;

void setup(){
// -----------------------------

  Serial.begin(115200);
  
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN); // Мощность (MIN для теста, MAX для улицы)
  radio.setDataRate(RF24_2MBPS); // Скорость
  radio.enableDynamicPayloads(); // Динамические пакеты
  radio.stopListening(); // Режим передатчика


// ------------------------------------
  packet.time = millis(); packet.ax = 0.05f; packet.ay = -0.03f; packet.az = 9.81f;
  packet.vx = 1.2f; packet.vy = -0.4f; packet.vz = 0.0f;
  packet.sx = 12.5f; packet.sy = -3.7f; packet.sz = 5.0f;
  packet.mx = 25.3f; packet.my = -12.1f; packet.mz = 45.6f;
  packet.H = 180.0f; packet.gx = 0.5f; packet.gy = -0.2f; packet.gz = 0.0f;
  packet.angx = 2.5f; packet.angy = -1.8f; packet.angz = 180.0f;
  packet.P = 101325.0f; packet.T = 23.5f; packet.BarAlt = 5.2f;
  packet.Voltage = 11.4f; packet.d = 15.8f; packet.Tcycle = 10.0f;
}
void loop()
{
// Указатель на начало структуры в памяти
  uint8_t* ptr = (uint8_t*)&packet;
  ChunkPayload chunk;

  // Отправляем структуру по кускам
  for (int i = 0; i < TOTAL_CHUNKS; i++) {
    chunk.chunk_id = i;
    
    // Считаем, сколько байт копировать в этот кусок
    int bytes_to_copy = (i == TOTAL_CHUNKS - 1) ? 
                        (sizeof(DataPacket) - (TOTAL_CHUNKS - 1) * CHUNK_DATA_SIZE) : 
                        CHUNK_DATA_SIZE;
                        
    // Копируем часть структуры в пакет
    memcpy(chunk.data, ptr + (i * CHUNK_DATA_SIZE), bytes_to_copy);
    
    // Отправляем пакет
    radio.write(&chunk, sizeof(ChunkPayload));
    
    delay(5); // Небольшая задержка, чтобы не переполнить FIFO модуля
  }
  
  delay(100); // Общая задержка между циклами отправки телеметрии
};
