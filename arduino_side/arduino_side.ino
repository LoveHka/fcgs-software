// arduino ino code for flight controller

#include <Wire.h>
#include <SPI.h>
#include <Servo.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "telemetry_protocol.h"   // Протокол для пакета передачи телеметрии

Servo Sch1;
Servo Sch2;
Servo Sch3;
Servo Sch4;
Servo ESCch1;

RF24 radio(9,10);
byte address[][6] = {"1Node","2Node","3Node","4Node","5Node","6Node"};
int data[7];

const byte ADXL345 = 0x53; //CS к питанию (5v на модуле есть стабилизатор)
const byte QMC5883L = 0x0D; 
const byte BMP180 = 0x77; 
const byte L3G4200D = 0x69; //SDO к 3.3v - 0x69, к GND - 0x68

//Раз в столько мс производятся вычисления скоростей, позиций и т.п.

float ax, ay, az;           // Ускорения, снятые с акселл. Если есть фильтр, то уже фильтрованные
float axCor, ayCor, azCor; // Коррекция акселерометра, устанавливается в функции AcsCall
float vx, vy, vz;         // Вычисленные скорости и координа
float sx, sy, sz;

// --- МАГНИТОМЕТР ---
int mx, my, mz;
float MagDecl = 11.93; //Магнитное склонение в градусах (оно изменчиво)
float Heading;

// --- ГИРОСКОП ---
float gx, gy, gz;
float angx, angy, angz;   // Углы поворота бпла в пространстве
float angrx, angry, angrz;  // Те же углы но в радианах 

// --- ТРМОМЕТР-БАРОМЕТР ---
int BarThermConstS[8];
unsigned int BarThermConstU[3];
const char* BarThermConstNameS[8] = {"AC1", "AC2", "AC3", "B1", "B2", "MB", "MC", "MD"};
const char* BarThermConstNameU[3] = {"AC4", "AC5", "AC6"};
int OSS = 0b11; //устанавливает соотношение передискретизации при измерении давления (00 - 1, 01 - 2, 10 - 4, 11 - 8) погрешности (0,6 гПа, 0,5 гПа, 0,4 гПа, 0,3 гПа) соответственно (гПа - гектопаскали)
byte BarThermMode[2] = {0x34 | (OSS<<6), 0x2E};
bool BarThermFlag = 1; //начинаем с температуры (Давление и температура измеряются поочерёдно, т.е. одно из них на 1 цикл!)
long Tbu;
long P;
float T;
int BarAlt;

unsigned long tmr;
byte Tcycle;//Раз в столько мс производятся вычисления скоростей, позиций и т.п.

void setup() {
  
  Serial.begin(115200);
  Wire.begin();

  AcsSetup();
  MagSetup();
  GyroSetup();
  BarThermSetup();

  RadioSetup();
  ActuatorsSetup();
  
  //Serial.println("Константы BarTherm двухбайтные, откуда при выводе берутся лишние 2 байта неясно");

}
unsigned long tmr2 = millis();

void loop() {

  tmr = millis();

  AcsCall();
  MagCall();
  GyroCall();
  BarThermCall();

  VelPos();

  RadioCall();
  ActuatorsCall();

  Tcycle = millis() - tmr;
  
  
  if(millis() - tmr2 >= 50){
    sendPacket();
    tmr2 = millis();
  }
}

static void writeU16LE(uint16_t v) // запись байта в сериал
{
    Serial.write((uint8_t)(v & 0xFF));
    Serial.write((uint8_t)(v >> 8));
}

// Отправка пакета с данными на комп
static void sendPacket()
{
    DataPacket p{};
    p.time   = (float)millis();

    p.az = az;  p.ay = ay;  p.ax = ax;
    p.vx = vx;  p.vy = vy;  p.vz = vz;
    p.sx = sx;  p.sy = sy;  p.sz = sz;

    p.mx = (float)mx;
    p.my = (float)my;
    p.mz = (float)mz;
    p.H  = Heading;

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

    Serial.write(TEL_SOF1);
    Serial.write(TEL_SOF2);
    Serial.write(TEL_VERSION);
    Serial.write(TEL_TYPE_DATA);

    writeU16LE(sizeof(DataPacket));
    Serial.write((const uint8_t*)&p, sizeof(p));

    uint16_t crc = crc16_ccitt((const uint8_t*)&p, sizeof(p));
    writeU16LE(crc);
}

void AcsSetup() {
  
  Wire.beginTransmission(ADXL345);
  Wire.write(byte(0x00)); //ID
  Wire.endTransmission();

  // прочитаем регистр
  Wire.requestFrom(ADXL345, 1);
  while (Wire.available()) {
    byte c = Wire.read();
    Serial.print("ACS ID = ");
    Serial.println(c, HEX);
  }

  Wire.beginTransmission(ADXL345);
  Wire.write(byte(0x31));
  Wire.write(byte(0x09)); // +-4g
  Wire.endTransmission();

  Wire.beginTransmission(ADXL345);
  Wire.write(byte(0x2D));
  Wire.write(byte(0x08)); // переведём акселерометр в режим измерений
  Wire.endTransmission();

}

void AcsCall() {
  
  
  Wire.beginTransmission(ADXL345);
  Wire.write(byte(0x32)); //адрес начала данных по осям X, Y и Z
  Wire.endTransmission();

  int x,y,z;

  float axCor = 0.2;
  float ayCor = 0;
  float azCor = 0;

  //прочитаем 6 байтов значений XYZ и преобразуем их
  Wire.requestFrom(ADXL345, 6);
  while (Wire.available()) {
    x = Wire.read(); //LSB  x 
    x |= Wire.read()<<8; //MSB  x
    y = Wire.read(); //LSB  z
    y |= Wire.read()<<8; //MSB z
    z = Wire.read(); //LSB y
    z |= Wire.read()<<8; //MSB y
  }

  ax = -(x/26.32 - axCor);
  ay = -(y/26.32 - ayCor);
  az = -(z/26.32 - azCor);

  // удалил ax=ax и для других осей.
}

void MagSetup() {
 
  Wire.beginTransmission(QMC5883L);
  Wire.write(byte(0x0D)); //ID
  Wire.endTransmission();


  Wire.requestFrom(QMC5883L, 1);
  while (Wire.available()) {
    byte c = Wire.read();
    Serial.print("MAG ID = ");
    Serial.println(c, HEX);
  }

  Wire.beginTransmission(QMC5883L); 
  Wire.write(0x09); // Set the Register
  Wire.write(0b00010101); // 00 - 512 Over Ratio; 01 - 8G Scale; 01 - 50 Hz Output Data Rate; 01 - Continuous mode
  Wire.endTransmission();

  Wire.beginTransmission(QMC5883L); //start talking
  Wire.write(0x0A); // Set the Register
  Wire.write(0b01000000); // крутим 0-6, остальное выкл.
  Wire.endTransmission();

}

void MagCall() {

  Wire.beginTransmission(QMC5883L);
  Wire.write(0x00); //адрес начала данных по осям X, Y и Z
  Wire.endTransmission();

  int x,y,z;
  
  //прочитаем 6 байтов значений XYZ и преобразуем их
  Wire.requestFrom(QMC5883L, 6);
  if(Wire.available()){
    x = Wire.read(); //LSB  x 
    x |= Wire.read()<<8; //MSB  x
    y = Wire.read(); //LSB  z
    y |= Wire.read()<<8; //MSB z
    z = Wire.read(); //LSB y
    z |= Wire.read()<<8; //MSB y
  }
                                  //Каждая единица в выводе это Scale (+-8 щас)/2^16, т.е. ~0.000244 Гаусс 0.244 млГаусс 4096 - 1 Гаусс
  mx = x;
  my = y;
  mz = z;

  float kMatrix[4][3] = {
    {1820.690037, 2054.211639, 1804.899320},
    {0.810047, -0.018315, -0.003823},
    {-0.018315, 1.030635, -0.013370},
    {-0.003823, -0.013370, 0.559138},
  };

  x = kMatrix[1][0]*(mx-kMatrix[0][0])+kMatrix[1][1]*(my-kMatrix[0][1])+kMatrix[1][2]*(mz-kMatrix[0][2]);
  y = kMatrix[2][0]*(mx-kMatrix[0][0])+kMatrix[2][1]*(my-kMatrix[0][1])+kMatrix[2][2]*(mz-kMatrix[0][2]);
  z = kMatrix[3][0]*(mx-kMatrix[0][0])+kMatrix[3][1]*(my-kMatrix[0][1])+kMatrix[3][2]*(mz-kMatrix[0][2]);

  //Serial.print(x); Serial.print(" "); Serial.print(y); Serial.print(" "); Serial.println(z); //Это для калибровки

  byte pitch = 0;
  byte roll = 0;
 
  x = x*cos(pitch) + y*sin(roll)*sin(pitch) + z*cos(roll)*sin(pitch);
  y = y*cos(roll) - z*sin(roll);
  Heading = atan2(y, x)/M_PI*180-MagDecl;
}

void GyroSetup() {

  Wire.beginTransmission(L3G4200D);
  Wire.write(byte(0x0F)); //ID
  Wire.endTransmission();

  Wire.requestFrom(L3G4200D, 1);
  while (Wire.available()) {
    byte c = Wire.read();
    Serial.print("GYRO ID = ");
    Serial.println(c, HEX);
  }

  byte GyroCTRL[5] = {0b00001111, 0b00101001, 0b00000000, 0b01000000, 0b00010000};

  // CTRL_REG1: 00 - ODR 100 Hz, 00 - Cut-off 12.5 Hz (режем выше этого), 1 - Normal mode, 111 - все оси 
  // CTRL_REG2: 00 - Резерв, 10 - Normal mode (для фильра высоких частот), 1001 - 0.01 Hz (режем ниже этого)
  // CTRL_REG3: 00000000 - Что-то про прерывания
  // CTRL_REG4: 0 - continous update (не ждём чтения), 1 - сначала LSB, потом MSB, 00 - 250 dps, 0000 - резерв, селфтест, SPI
  // CTRL_REG5: 0 - Normal mode (память), 0 - FIFO disable, 0 - надо, 1 - High Pass filter вкл., 0000 - прерывания лимиты фигня

  for (int i = 0; i < 5; i++) {
     Wire.beginTransmission(L3G4200D);
     Wire.write(0x20+i); //адрес начала константы со знаком ч.1
     Wire.write(GyroCTRL[i]);
     Wire.endTransmission();
   } 
  // НАчальные значения углов. Не скоростей.
  angx = 0;
  angy = 0;
  angz = 90;

}


void GyroCall() {

  int x,y,z;
  
  Wire.beginTransmission(L3G4200D);
  Wire.write(0x28); //адрес начала данных по оси X
  Wire.endTransmission();

  //прочитаем 2 байта значений X и преобразуем их, я хз почему нельяз брать сразу 6, но при этот он выдаёт значения одной оси на все 3
  Wire.requestFrom(L3G4200D, 2);
  if(Wire.available()){
    x = Wire.read(); //LSB  x 
    x |= Wire.read()<<8; //MSB  x
  }

  Wire.beginTransmission(L3G4200D);
  Wire.write(0x2A); //адрес начала данных по оси Y
  Wire.endTransmission();

  //прочитаем 2 байта значений Y и преобразуем их
  Wire.requestFrom(L3G4200D, 2);
  if(Wire.available()){
    y = Wire.read(); //LSB  y 
    y |= Wire.read()<<8; //MSB  y
  }

  Wire.beginTransmission(L3G4200D);
  Wire.write(0x2C); //адрес начала данных по оси Z
  Wire.endTransmission();

  //прочитаем 2 байта значений Z и преобразуем их
  Wire.requestFrom(L3G4200D, 2);
  if(Wire.available()){
    z = Wire.read(); //LSB  z 
    z |= Wire.read()<<8; //MSB  z
  }
 
  gx = x/131.072;
  gy = y/131.072;
  gz = z/131.072;

}


void BarThermSetup() {
  
  Wire.beginTransmission(BMP180);
  Wire.write(byte(0xD0)); //ID
  Wire.endTransmission();

  Wire.requestFrom(BMP180, 1);
  while (Wire.available()) {
    byte c = Wire.read();
    Serial.print("BT ID = ");
    Serial.println(c, HEX);
  }

  for (int i = 0; i < 3; i++) {
    
    Wire.beginTransmission(BMP180);
    Wire.write(0xAA+i*2); //адрес начала константы со знаком ч.1
    Wire.endTransmission();

    //прочитаем MSB и LSB для каждой константы и выведем значения
    Wire.requestFrom(BMP180, 2);
    if(Wire.available()){
      BarThermConstS[i] = Wire.read()<<8; //MSB
      BarThermConstS[i] |= Wire.read(); //LSB
      Serial.print(BarThermConstNameS[i]); Serial.print(" = "); Serial.println(BarThermConstS[i], DEC); //Serial.print("\t"); Serial.println(sizeof(BarThermConstS[i]));
    }
  }

   for (int i = 0; i < 3; i++) {
 
    Wire.beginTransmission(BMP180);
    Wire.write(0xB0+i*2); //адрес начала констант без знака
    Wire.endTransmission();

    //прочитаем MSB и LSB для каждой константы и выведем значения
    Wire.requestFrom(BMP180, 2);
    if(Wire.available()){
      BarThermConstU[i] = Wire.read()<<8; //MSB
      BarThermConstU[i] |= Wire.read(); //LSB
      Serial.print(BarThermConstNameU[i]); Serial.print(" = "); Serial.println(BarThermConstU[i], DEC); //Serial.print("\t"); Serial.println(sizeof(BarThermConstU[i]));
    }    
  }

  for (int i = 3; i < 8; i++) {
 
    Wire.beginTransmission(BMP180);
    Wire.write(0xB6+(i-3)*2); //адрес начала константы со знаком ч.2
    Wire.endTransmission();

    //прочитаем MSB и LSB для каждой константы и выведем значения
    Wire.requestFrom(BMP180, 2);
    if(Wire.available()){
      BarThermConstS[i] = Wire.read()<<8; //MSB
      BarThermConstS[i] |= Wire.read(); //LSB
      Serial.print(BarThermConstNameS[i]); Serial.print(" = "); Serial.println(BarThermConstS[i], DEC); //Serial.print("\t"); Serial.println(sizeof(BarThermConstS[i]));
    }
  }
}



void BarThermCall() {

  long UT; 
  long UP;

  Wire.beginTransmission(BMP180);
  Wire.write(byte(0xF6)); //регистр данных
  Wire.endTransmission();

  //прочитаем давление
  if (BarThermFlag == 0) {
    Wire.requestFrom(BMP180, 3);
    while (Wire.available()) {
      UP = (long)Wire.read()<<16; //MSB
      UP |= (long)Wire.read()<<8; //LSB
      UP |= (long)Wire.read(); //XLSB
      UP >>= (long)8 - OSS; //зависимость от выбранной точности

      long Pp;
         
      long B6 = Tbu-4000;
      long X1 = ((long)BarThermConstS[4]*((long)B6*B6>>12))>>11;
      long X2 = (long)BarThermConstS[1]*B6>>11;
      long X3 = X1+X2;
      long B3 = (((long)((long)BarThermConstS[0]*4+X3)<<OSS)+2)>>2; 
           X1 = (long)BarThermConstS[2]*B6>>13; 
           X2 = ((long)BarThermConstS[3]*((long)B6*B6>>12))>>16;
           X3 = ((X1+X2)+2)>>2; 
      unsigned long B4 = BarThermConstU[0]*(unsigned long)(X3 + 32768)>>15; //ок
      unsigned long B7 = (unsigned long)((unsigned long)UP-B3)*(50000>>OSS);
      if (B7 < 0x80000000) {P = (unsigned long)((unsigned long)B7*2)/B4;} else {P = (unsigned long)((unsigned long)B7/B4)<<1;}       
           X1 = (long)(P>>8)*(P>>8);
           X1 = ((long)X1*3038)>>16;
           X2 = ((long)-7357*P)>>16;
           P += (X1+X2+3791)>>4;
           BarAlt = (float)44330*(1-pow(P/101325.0, 1.0/5.255));
    }
  }

  //прочитаем температуру
  if (BarThermFlag == 1) {
    Wire.requestFrom(BMP180, 2);
    while (Wire.available()) {
      UT = Wire.read()<<8; //MSB
      UT |= Wire.read(); //LSB

      long X1 = (UT - BarThermConstU[2])*BarThermConstU[1]>>15;
      long X2 = ((long)BarThermConstS[6]<<11)/(X1+BarThermConstS[7]);
      long B5 = X1+X2;
      T = ((float)B5+8)/160;

      Tbu = B5;
    }
  }

  BarThermFlag = !BarThermFlag;
    
  Wire.beginTransmission(BMP180);
  Wire.write(byte(0xF4)); //регистр настроек
  Wire.write(BarThermMode[BarThermFlag]); //чё измеряем 
  Wire.endTransmission();

}

void RadioSetup() {

  radio.begin();

  radio.setAutoAck(0); 
  radio.setRetries(0, 15);   
  radio.enableAckPayload();
  radio.setPayloadSize(32);
  
  radio.setChannel(0x00);
  radio.openReadingPipe(1, address[0]);
  
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
  radio.powerUp();
  radio.startListening();
  
}

void RadioCall() {

  if(radio.available()) { 
      radio.read( &data, sizeof(data));
  }
  
}

void ActuatorsSetup() {

  pinMode(3, OUTPUT);
  pinMode(5, OUTPUT); //Н-мост НЕ ОТКРЫВАТЬ ДВА СРАЗУ!!!
  pinMode(6, OUTPUT); //Н-мост НЕ ОТКРЫВАТЬ ДВА СРАЗУ!!!
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
  Sch4.write(abs((int)Heading)); //data[4]
  
}

void VelPos() {

    float tcstep = (float) Tcycle / 1000;

    vx = vx + ax * tcstep;
    vy = vy + ay * tcstep;
    vz = vz + az * tcstep;

    sx = sx + vx * tcstep;
    sy = sy + vy * tcstep;
    sz = sz + vz * tcstep;

    angx = angx + gx * tcstep;
    angy = angy + gy * tcstep;
    angz = angz + gz * tcstep;

    angrx = angx * PI / 180;
    angry = angy * PI / 180;
    angrz = angz  * PI / 180;
 
}
