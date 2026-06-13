// arduino ino code for flight controller

#include <Wire.h>
#include <SPI.h>
#include <Servo.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <math.h>
#include "telemetry_protocol.h"

Servo Sch1;
Servo Sch2;
Servo Sch3;
Servo Sch4;
Servo ESCch1;

RF24 radio(9,10);
byte address[6] = "Node1";

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

float received_calib[12] = {0};
bool calib_chunk0_received = false;
bool calib_chunk1_received = false;

int data[7];
uint8_t currentMode = MODE_NORMAL;

const byte ADXL345 = 0x53;
const byte QMC5883L = 0x0D; 
const byte BMP180 = 0x77; 
const byte L3G4200D = 0x69;

float ax, ay, az;
float axCor, ayCor, azCor;
float vx, vy, vz;
float sx, sy, sz;

float mx, my, mz;
float MagDecl = 12.42;
float Heading;
float magOffset[3] = {0, 0, 0};
float magMatrix[3][3] = {
  {1, 0, 0},
  {0, 1, 0},
  {0, 0, 1}
};

float gx, gy, gz;
float angx, angy, angz;
float angrx, angry, angrz;

int BarThermConstS[8];
unsigned int BarThermConstU[3];
const char* BarThermConstNameS[8] = {"AC1", "AC2", "AC3", "B1", "B2", "MB", "MC", "MD"};
const char* BarThermConstNameU[3] = {"AC4", "AC5", "AC6"};
int OSS = 0b11;
byte BarThermMode[2] = {0x34 | (OSS<<6), 0x2E};
bool BarThermFlag = 1;
long Tbu;
long P;
float T;
int BarAlt;

unsigned long tmr;
byte Tcycle;
float tcstep = 1;

float beta = 5.0f;
float zeta = 0.0f;
float SEq_1 = 1.0f, SEq_2 = 0.0f, SEq_3 = 0.0f, SEq_4 = 0.0f;
float b_x = 1.0f, b_z = 0.0f;
float w_bx = 0.0f, w_by = 0.0f, w_bz = 0.0f;

float acc_alpha = 0.1;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  AcsSetup();
  MagSetup();
  GyroSetup();
  BarThermSetup();

  RadioSetup();
}

unsigned long tmr2 = millis();
unsigned long lastBarTime = 0;
const uint32_t BAR_INTERVAL = 30;

void loop() {
  tmr = millis();
  
  processIncoming();

  AcsCall();
  MagCall();
  GyroCall();
  if (millis() - lastBarTime >= BAR_INTERVAL){
    BarThermCall();
    lastBarTime = millis();
  }
  Tcycle = millis() - tmr;
  tcstep = (float) Tcycle / 1000;

  if (currentMode == MODE_NORMAL){
    filterUpdate(gx, gy, gz, ax, ay, az, mx, my, mz);
    VelPos();
  }
    
  sendPacket();
  RadioCall();
}

static void writeU16LE(uint16_t v) {
    Serial.write((uint8_t)(v & 0xFF));
    Serial.write((uint8_t)(v >> 8));
}

static void sendPacket() {
    DataPacket p{};
    p.time = (float)millis();

    p.az = az;  p.ay = ay;  p.ax = ax;
    p.vx = vx;  p.vy = vy;  p.vz = vz;
    p.sx = sx;  p.sy = sy;  p.sz = sz;

    p.mx = mx;
    p.my = my;
    p.mz = mz;
    p.H = Heading;

    p.gx = gx;  p.gy = gy;  p.gz = gz;

    p.angx = angx;
    p.angy = angy;
    p.angz = angz;

    p.P = (float)P;
    p.T = T;
    p.BarAlt = (float)BarAlt;
    p.Voltage = (float)(analogRead(7) * 5.0f) / 1024.0f;
    p.d = (float)data[1];
    p.Tcycle = (float)Tcycle;

    uint8_t* ptr = (uint8_t*)&p;
    ChunkPayload chunk;

    for (int i = 0; i < TOTAL_CHUNKS; i++) {
        chunk.chunk_id = i;
        
        int bytes_to_copy = (i == TOTAL_CHUNKS - 1) ? 
                            (sizeof(DataPacket) - (TOTAL_CHUNKS - 1) * CHUNK_DATA_SIZE) : 
                            CHUNK_DATA_SIZE;
                            
        memcpy(chunk.data, ptr + (i * CHUNK_DATA_SIZE), bytes_to_copy);
        
        radio.write(&chunk, sizeof(ChunkPayload));
        
        delay(5);
    }
}

void processIncoming() {
    static uint8_t state = 0;
    static uint16_t len = 0;
    static uint8_t type = 0;
    static uint8_t payload[128];
    static uint16_t idx = 0;

    while (Serial.available()) {
        uint8_t b = Serial.read();

        switch (state) {
            case 0:
                if (b == TEL_SOF1) state = 1;
                break;

            case 1:
                if (b == TEL_SOF2) state = 2;
                else state = 0;
                break;

            case 2:
                state = 3;
                break;

            case 3:
                type = b;
                state = 4;
                break;

            case 4:
                len = b;
                state = 5;
                break;

            case 5:
                len |= (uint16_t)b << 8;
                idx = 0;
                state = 6;
                break;

            case 6:
                payload[idx++] = b;
                if (idx >= len) state = 7;
                break;

            case 7:
                state = 0;
                sendMessage(payload);
                if (type == TEL_TYPE_CMD && len == sizeof(CommandPacket)) {
                    CommandPacket cmd;
                    memcpy(&cmd, payload, sizeof(cmd));
                    handleCommand(cmd);
                }
                break;
        }
    }
}

void handleCommand(const CommandPacket& c) {
    switch (c.cmd) {
        case CMD_SET_MODE:
            currentMode = (uint8_t)c.params[0];
            if (currentMode == MODE_MAG_CALIB){
                float magOffset[3] = {0, 0, 0};
                float magMatrix[3][3] = {
                    {1, 0, 0},
                    {0, 1, 0},
                    {0, 0, 1}
                };
            }
            break;

        case CMD_SET_MAG_CALIB:
            magOffset[0] = c.params[0];
            magOffset[1] = c.params[1];
            magOffset[2] = c.params[2];
            int k = 3;
            for (int r = 0; r < 3; r++)
                for (int col = 0; col < 3; col++)
                    magMatrix[r][col] = c.params[k++];
            
            sendMessage("Я получил вашу матрицу! Ня!");
            break;
    }
}

void AcsSetup() {
    Wire.beginTransmission(ADXL345);
    Wire.write(byte(0x00));
    Wire.endTransmission();

    Wire.requestFrom(ADXL345, 1);
    while (Wire.available()) {
        byte c = Wire.read();
        Serial.print("ACS ID = ");
        Serial.println(c, HEX);
    }

    Wire.beginTransmission(ADXL345);
    Wire.write(byte(0x31));
    Wire.write(byte(0x09));
    Wire.endTransmission();

    Wire.beginTransmission(ADXL345);
    Wire.write(byte(0x2D));
    Wire.write(byte(0x08));
    Wire.endTransmission();
}

void AcsCall() {
    Wire.beginTransmission(ADXL345);
    Wire.write(byte(0x32));
    Wire.endTransmission();

    int x,y,z;

    axCor = 0.2;
    ayCor = 0;
    azCor = 0;

    Wire.requestFrom(ADXL345, 6);
    while (Wire.available()) {
        x = Wire.read();
        x |= Wire.read()<<8;
        y = Wire.read();
        y |= Wire.read()<<8;
        z = Wire.read();
        z |= Wire.read()<<8;
    }
    float nax, nay, naz; 
    nax = -(x/26.32 - axCor);
    nay = (y/26.32 - ayCor);
    naz = (z/26.32 - azCor);
    
    ax = acc_alpha * nax + (1 - acc_alpha) * ax;
    ay = acc_alpha * nay + (1 - acc_alpha) * ay;
    az = acc_alpha * naz + (1 - acc_alpha) * az;
}

void MagSetup() {
    Wire.beginTransmission(QMC5883L);
    Wire.write(byte(0x0D));
    Wire.endTransmission();

    Wire.requestFrom(QMC5883L, 1);
    while (Wire.available()) {
        byte c = Wire.read();
        Serial.print("MAG ID = ");
        Serial.println(c, HEX);
    }

    Wire.beginTransmission(QMC5883L); 
    Wire.write(0x09);
    Wire.write(0b00010101);
    Wire.endTransmission();

    Wire.beginTransmission(QMC5883L);
    Wire.write(0x0A);
    Wire.write(0b01000000);
    Wire.endTransmission();
}

void MagCall() {
    Wire.beginTransmission(QMC5883L);
    Wire.write(0x00);
    Wire.endTransmission();

    int x,y,z;
    
    Wire.requestFrom(QMC5883L, 6);
    if(Wire.available()){
        x = Wire.read();
        x |= Wire.read()<<8;
        y = Wire.read();
        y |= Wire.read()<<8;
        z = Wire.read();
        z |= Wire.read()<<8;
    }
    
    float fx = x;
    float fy = -y;
    float fz = -z;
    if (currentMode == MODE_NORMAL) {
        applyMagCalibration(fx, fy, fz);
    }
    mx = fx;
    my = fy;
    mz = fz;

    Heading = atan2((float)(-my), (float)mx) * 180.0 / PI;
    Heading -= MagDecl;
    if (Heading < 0) Heading += 360.0;
    if (Heading >= 360.0) Heading -= 360.0;
}

void applyMagCalibration(float& x, float& y, float& z) {
    float rx = x - magOffset[0];
    float ry = y - magOffset[1];
    float rz = z - magOffset[2];

    float cx = magMatrix[0][0] * rx + magMatrix[0][1] * ry + magMatrix[0][2] * rz;
    float cy = magMatrix[1][0] * rx + magMatrix[1][1] * ry + magMatrix[1][2] * rz;
    float cz = magMatrix[2][0] * rx + magMatrix[2][1] * ry + magMatrix[2][2] * rz;

    x = cx;
    y = cy;
    z = cz;
}

void GyroSetup() {
    Wire.beginTransmission(L3G4200D);
    Wire.write(byte(0x0F));
    Wire.endTransmission();

    Wire.requestFrom(L3G4200D, 1);
    while (Wire.available()) {
        byte c = Wire.read();
        Serial.print("GYRO ID = ");
        Serial.println(c, HEX);
    }

    byte GyroCTRL[5] = {0b00001111, 0b00101001, 0b00000000, 0b01000000, 0b00010000};

    for (int i = 0; i < 5; i++) {
        Wire.beginTransmission(L3G4200D);
        Wire.write(0x20+i);
        Wire.write(GyroCTRL[i]);
        Wire.endTransmission();
    } 
    
    angx = 0;
    angy = 0;
    angz = 90;
}

void GyroCall() {
    int x,y,z;
    
    Wire.beginTransmission(L3G4200D);
    Wire.write(0x28);
    Wire.endTransmission();

    Wire.requestFrom(L3G4200D, 2);
    if(Wire.available()){
        x = Wire.read();
        x |= Wire.read()<<8;
    }

    Wire.beginTransmission(L3G4200D);
    Wire.write(0x2A);
    Wire.endTransmission();

    Wire.requestFrom(L3G4200D, 2);
    if(Wire.available()){
        y = Wire.read();
        y |= Wire.read()<<8;
    }

    Wire.beginTransmission(L3G4200D);
    Wire.write(0x2C);
    Wire.endTransmission();

    Wire.requestFrom(L3G4200D, 2);
    if(Wire.available()){
        z = Wire.read();
        z |= Wire.read()<<8;
    }
    
    gx = x/131.072;
    gy = -y/131.072;
    gz = -z/131.072;
}

void BarThermSetup() {
    Wire.beginTransmission(BMP180);
    Wire.write(byte(0xD0));
    Wire.endTransmission();

    Wire.requestFrom(BMP180, 1);
    while (Wire.available()) {
        byte c = Wire.read();
        Serial.print("BT ID = ");
        Serial.println(c, HEX);
    }

    for (int i = 0; i < 3; i++) {
        Wire.beginTransmission(BMP180);
        Wire.write(0xAA+i*2);
        Wire.endTransmission();

        Wire.requestFrom(BMP180, 2);
        if(Wire.available()){
            BarThermConstS[i] = Wire.read()<<8;
            BarThermConstS[i] |= Wire.read();
            Serial.print(BarThermConstNameS[i]); Serial.print(" = "); Serial.println(BarThermConstS[i], DEC);
        }
    }

    for (int i = 0; i < 3; i++) {
        Wire.beginTransmission(BMP180);
        Wire.write(0xB0+i*2);
        Wire.endTransmission();

        Wire.requestFrom(BMP180, 2);
        if(Wire.available()){
            BarThermConstU[i] = Wire.read()<<8;
            BarThermConstU[i] |= Wire.read();
            Serial.print(BarThermConstNameU[i]); Serial.print(" = "); Serial.println(BarThermConstU[i], DEC);
        }    
    }

    for (int i = 3; i < 8; i++) {
        Wire.beginTransmission(BMP180);
        Wire.write(0xB6+(i-3)*2);
        Wire.endTransmission();

        Wire.requestFrom(BMP180, 2);
        if(Wire.available()){
            BarThermConstS[i] = Wire.read()<<8;
            BarThermConstS[i] |= Wire.read();
            Serial.print(BarThermConstNameS[i]); Serial.print(" = "); Serial.println(BarThermConstS[i], DEC);
        }
    }
}

void BarThermCall() {
    long UT; 
    long UP;

    Wire.beginTransmission(BMP180);
    Wire.write(byte(0xF6));
    Wire.endTransmission();

    if (BarThermFlag == 0) {
        Wire.requestFrom(BMP180, 3);
        while (Wire.available()) {
            UP = (long)Wire.read()<<16;
            UP |= (long)Wire.read()<<8;
            UP |= (long)Wire.read();
            UP >>= (long)8 - OSS;

            long Pp;
            
            long B6 = Tbu-4000;
            long X1 = ((long)BarThermConstS[4]*((long)B6*B6>>12))>>11;
            long X2 = (long)BarThermConstS[1]*B6>>11;
            long X3 = X1+X2;
            long B3 = (((long)((long)BarThermConstS[0]*4+X3)<<OSS)+2)>>2; 
            X1 = (long)BarThermConstS[2]*B6>>13; 
            X2 = ((long)BarThermConstS[3]*((long)B6*B6>>12))>>16;
            X3 = ((X1+X2)+2)>>2; 
            unsigned long B4 = BarThermConstU[0]*(unsigned long)(X3 + 32768)>>15;
            unsigned long B7 = (unsigned long)((unsigned long)UP-B3)*(50000>>OSS);
            if (B7 < 0x80000000) {P = (unsigned long)((unsigned long)B7*2)/B4;} else {P = (unsigned long)((unsigned long)B7/B4)<<1;}       
            X1 = (long)(P>>8)*(P>>8);
            X1 = ((long)X1*3038)>>16;
            X2 = ((long)-7357*P)>>16;
            P += (X1+X2+3791)>>4;
            BarAlt = (float)44330*(1-pow(P/101325.0, 1.0/5.255));
        }
    }

    if (BarThermFlag == 1) {
        Wire.requestFrom(BMP180, 2);
        while (Wire.available()) {
            UT = Wire.read()<<8;
            UT |= Wire.read();

            long X1 = (UT - BarThermConstU[2])*BarThermConstU[1]>>15;
            long X2 = ((long)BarThermConstS[6]<<11)/(X1+BarThermConstS[7]);
            long B5 = X1+X2;
            T = ((float)B5+8)/160;

            Tbu = B5;
        }
    }

    BarThermFlag = !BarThermFlag;
    
    Wire.beginTransmission(BMP180);
    Wire.write(byte(0xF4));
    Wire.write(BarThermMode[BarThermFlag]);
    Wire.endTransmission();
}

void RadioSetup() {
    radio.begin();
    radio.openWritingPipe(address);
    radio.setPALevel(RF24_PA_MIN);
    radio.setDataRate(RF24_2MBPS);
    radio.enableDynamicPayloads();
    radio.enableAckPayload();
    radio.stopListening();
}

void RadioCall() {
    // ИСПРАВЛЕНО: while вместо if для чтения всех доступных ACK
    while (radio.isAckPayloadAvailable()) {
        MagCalibChunk ack;
        radio.read(&ack, sizeof(ack));

        if (ack.magic == CMD_CHUNK_MAGIC) {
            if (ack.chunk_id == 0) {
                memcpy(received_calib, ack.data, 6 * sizeof(float));
                calib_chunk0_received = true;
            } else if (ack.chunk_id == 1) {
                memcpy(received_calib + 6, ack.data, 6 * sizeof(float));
                calib_chunk1_received = true;
            }

            if (calib_chunk0_received && calib_chunk1_received) {
                magOffset[0] = received_calib[0];
                magOffset[1] = received_calib[1];
                magOffset[2] = received_calib[2];

                int k = 3;
                for (int r = 0; r < 3; r++)
                    for (int c = 0; c < 3; c++)
                        magMatrix[r][c] = received_calib[k++];

                calib_chunk0_received = false;
                calib_chunk1_received = false;

                sendMessage("Калибровка магнитометра применена!");
            }
        }
    }
}

void ActuatorsSetup() {
    pinMode(3, OUTPUT);
    pinMode(5, OUTPUT);
    pinMode(6, OUTPUT);
    pinMode(14, OUTPUT);
    pinMode(15, OUTPUT);
    pinMode(16, OUTPUT);
    
    Sch1.attach(2);
    Sch2.attach(4);
    Sch3.attach(7);
    Sch4.attach(8);
    ESCch1.attach(17);
}

void ActuatorsCall() {
    Sch1.write(data[1]);
    Sch2.write(data[2]);
    Sch3.write(data[3]);
    Sch4.write(abs((int)Heading));
}

void VelPos() {
    vx = vx + ax * tcstep;
    vy = vy + ay * tcstep;
    vz = vz + az * tcstep;

    sx = sx + vx * tcstep;
    sy = sy + vy * tcstep;
    sz = sz + vz * tcstep;
    
    angx = atan2(2.0f * (SEq_3 * SEq_4 + SEq_1 * SEq_2), SEq_1 * SEq_1 - SEq_2 * SEq_2 - SEq_3 * SEq_3 + SEq_4 * SEq_4) * 180.0f / PI;
    angy = asin(-2.0f * (SEq_2 * SEq_4 - SEq_1 * SEq_3)) * 180.0f / PI;
    angz = atan2(2.0f * (SEq_2 * SEq_3 + SEq_1 * SEq_4), SEq_1 * SEq_1 + SEq_2 * SEq_2 - SEq_3 * SEq_3 - SEq_4 * SEq_4) * 180.0f / PI;

    angrx = angx * PI / 180;
    angry = angy * PI / 180;
    angrz = angz * PI / 180;
}

void sendMessage(const char* msg) {
    uint16_t len = strlen(msg);
    if (len > MAX_MSG_LEN) len = MAX_MSG_LEN;
    
    Serial.write(TEL_SOF1);
    Serial.write(TEL_SOF2);
    Serial.write(TEL_VERSION);
    Serial.write(TEL_TYPE_MESSAGE);
    Serial.write(len & 0xFF);
    Serial.write(len >> 8);
    
    Serial.write(reinterpret_cast<const uint8_t*>(msg), len);
    
    uint16_t crc = crc16_ccitt(reinterpret_cast<const uint8_t*>(msg), len);
    Serial.write(crc & 0xFF);
    Serial.write(crc >> 8);
}

void filterUpdate(float w_x, float w_y, float w_z, float a_x, float a_y, float a_z, float m_x, float m_y, float m_z) {
    w_x = w_x * PI / 180.0f;
    w_y = w_y * PI / 180.0f;
    w_z = w_z * PI / 180.0f;
    
    float norm;
    float SEqDot_omega_1, SEqDot_omega_2, SEqDot_omega_3, SEqDot_omega_4;
    float f_1, f_2, f_3, f_4, f_5, f_6;
    float J_11or24, J_12or23, J_13or22, J_14or21, J_32, J_33,
          J_41, J_42, J_43, J_44, J_51, J_52, J_53, J_54, J_61, J_62, J_63, J_64;
    float SEqHatDot_1, SEqHatDot_2, SEqHatDot_3, SEqHatDot_4;
    float w_err_x, w_err_y, w_err_z;
    float h_x, h_y, h_z;

    float halfSEq_1 = 0.5f * SEq_1;
    float halfSEq_2 = 0.5f * SEq_2;
    float halfSEq_3 = 0.5f * SEq_3;
    float halfSEq_4 = 0.5f * SEq_4;
    float twoSEq_1 = 2.0f * SEq_1;
    float twoSEq_2 = 2.0f * SEq_2;
    float twoSEq_3 = 2.0f * SEq_3;
    float twoSEq_4 = 2.0f * SEq_4;
    float twob_x = 2.0f * b_x;
    float twob_z = 2.0f * b_z;
    float twob_xSEq_1 = 2.0f * b_x * SEq_1;
    float twob_xSEq_2 = 2.0f * b_x * SEq_2;
    float twob_xSEq_3 = 2.0f * b_x * SEq_3;
    float twob_xSEq_4 = 2.0f * b_x * SEq_4;
    float twob_zSEq_1 = 2.0f * b_z * SEq_1;
    float twob_zSEq_2 = 2.0f * b_z * SEq_2;
    float twob_zSEq_3 = 2.0f * b_z * SEq_3;
    float twob_zSEq_4 = 2.0f * b_z * SEq_4;
    float SEq_1SEq_2;
    float SEq_1SEq_3 = SEq_1 * SEq_3;
    float SEq_1SEq_4;
    float SEq_2SEq_3;
    float SEq_2SEq_4 = SEq_2 * SEq_4;
    float SEq_3SEq_4;
    float twom_x = 2.0f * m_x;
    float twom_y = 2.0f * m_y;
    float twom_z = 2.0f * m_z;

    norm = sqrt(a_x * a_x + a_y * a_y + a_z * a_z);
    a_x /= norm;
    a_y /= norm;
    a_z /= norm;

    norm = sqrt(m_x * m_x + m_y * m_y + m_z * m_z);
    m_x /= norm;
    m_y /= norm;
    m_z /= norm;

    f_1 = twoSEq_2 * SEq_4 - twoSEq_1 * SEq_3 - a_x;
    f_2 = twoSEq_1 * SEq_2 + twoSEq_3 * SEq_4 - a_y;
    f_3 = 1.0f - twoSEq_2 * SEq_2 - twoSEq_3 * SEq_3 - a_z;
    f_4 = twob_x * (0.5f - SEq_3 * SEq_3 - SEq_4 * SEq_4) + twob_z * (SEq_2SEq_4 - SEq_1SEq_3) - m_x;
    f_5 = twob_x * (SEq_2 * SEq_3 - SEq_1 * SEq_4) + twob_z * (SEq_1 * SEq_2 + SEq_3 * SEq_4) - m_y;
    f_6 = twob_x * (SEq_1SEq_3 + SEq_2SEq_4) + twob_z * (0.5f - SEq_2 * SEq_2 - SEq_3 * SEq_3) - m_z;

    J_11or24 = twoSEq_3;
    J_12or23 = 2.0f * SEq_4;
    J_13or22 = twoSEq_1;
    J_14or21 = twoSEq_2;
    J_32 = 2.0f * J_14or21;
    J_33 = 2.0f * J_11or24;
    J_41 = twob_zSEq_3;
    J_42 = twob_zSEq_4;
    J_43 = 2.0f * twob_xSEq_3 + twob_zSEq_1;
    J_44 = 2.0f * twob_xSEq_4 - twob_zSEq_2;
    J_51 = twob_xSEq_4 - twob_zSEq_2;
    J_52 = twob_xSEq_3 + twob_zSEq_1;
    J_53 = twob_xSEq_2 + twob_zSEq_4;
    J_54 = twob_xSEq_1 - twob_zSEq_3;
    J_61 = twob_xSEq_3;
    J_62 = twob_xSEq_4 - 2.0f * twob_zSEq_2;
    J_63 = twob_xSEq_1 - 2.0f * twob_zSEq_3;
    J_64 = twob_xSEq_2;

    SEqHatDot_1 = J_14or21 * f_2 - J_11or24 * f_1 - J_41 * f_4 - J_51 * f_5 + J_61 * f_6;
    SEqHatDot_2 = J_12or23 * f_1 + J_13or22 * f_2 - J_32 * f_3 + J_42 * f_4 + J_52 * f_5 + J_62 * f_6;
    SEqHatDot_3 = J_12or23 * f_2 - J_33 * f_3 - J_13or22 * f_1 - J_43 * f_4 + J_53 * f_5 + J_63 * f_6;
    SEqHatDot_4 = J_14or21 * f_1 + J_11or24 * f_2 - J_44 * f_4 - J_54 * f_5 + J_64 * f_6;

    norm = sqrt(SEqHatDot_1 * SEqHatDot_1 + SEqHatDot_2 * SEqHatDot_2 + SEqHatDot_3 * SEqHatDot_3 + SEqHatDot_4 * SEqHatDot_4);
    SEqHatDot_1 /= norm;
    SEqHatDot_2 /= norm;
    SEqHatDot_3 /= norm;
    SEqHatDot_4 /= norm;

    w_err_x = twoSEq_1 * SEqHatDot_2 - twoSEq_2 * SEqHatDot_1 - twoSEq_3 * SEqHatDot_4 + twoSEq_4 * SEqHatDot_3;
    w_err_y = twoSEq_1 * SEqHatDot_3 + twoSEq_2 * SEqHatDot_4 - twoSEq_3 * SEqHatDot_1 - twoSEq_4 * SEqHatDot_2;
    w_err_z = twoSEq_1 * SEqHatDot_4 - twoSEq_2 * SEqHatDot_3 + twoSEq_3 * SEqHatDot_2 - twoSEq_4 * SEqHatDot_1;

    w_bx += w_err_x * tcstep * zeta;
    w_by += w_err_y * tcstep * zeta;
    w_bz += w_err_z * tcstep * zeta;
    w_x -= w_bx;
    w_y -= w_by;
    w_z -= w_bz;

    SEqDot_omega_1 = -halfSEq_2 * w_x - halfSEq_3 * w_y - halfSEq_4 * w_z;
    SEqDot_omega_2 = halfSEq_1 * w_x + halfSEq_3 * w_z - halfSEq_4 * w_y;
    SEqDot_omega_3 = halfSEq_1 * w_y - halfSEq_2 * w_z + halfSEq_4 * w_x;
    SEqDot_omega_4 = halfSEq_1 * w_z + halfSEq_2 * w_y - halfSEq_3 * w_x;

    SEq_1 += (SEqDot_omega_1 - (beta * SEqHatDot_1)) * tcstep;
    SEq_2 += (SEqDot_omega_2 - (beta * SEqHatDot_2)) * tcstep;
    SEq_3 += (SEqDot_omega_3 - (beta * SEqHatDot_3)) * tcstep;
    SEq_4 += (SEqDot_omega_4 - (beta * SEqHatDot_4)) * tcstep;

    norm = sqrt(SEq_1 * SEq_1 + SEq_2 * SEq_2 + SEq_3 * SEq_3 + SEq_4 * SEq_4);
    SEq_1 /= norm;
    SEq_2 /= norm;
    SEq_3 /= norm;
    SEq_4 /= norm;

    SEq_1SEq_2 = SEq_1 * SEq_2;
    SEq_1SEq_3 = SEq_1 * SEq_3;
    SEq_1SEq_4 = SEq_1 * SEq_4;
    SEq_3SEq_4 = SEq_3 * SEq_4;
    SEq_2SEq_3 = SEq_2 * SEq_3;
    SEq_2SEq_4 = SEq_2 * SEq_4;
    h_x = twom_x * (0.5f - SEq_3 * SEq_3 - SEq_4 * SEq_4) + twom_y * (SEq_2SEq_3 - SEq_1SEq_4) + twom_z * (SEq_2SEq_4 + SEq_1SEq_3);
    h_y = twom_x * (SEq_2SEq_3 + SEq_1SEq_4) + twom_y * (0.5f - SEq_2 * SEq_2 - SEq_4 * SEq_4) + twom_z * (SEq_3SEq_4 - SEq_1SEq_2);
    h_z = twom_x * (SEq_2SEq_4 - SEq_1SEq_3) + twom_y * (SEq_3SEq_4 + SEq_1SEq_2) + twom_z * (0.5f - SEq_2 * SEq_2 - SEq_3 * SEq_3);

    b_x = sqrt((h_x * h_x) + (h_y * h_y));
    b_z = h_z;
}
