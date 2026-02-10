#pragma once

struct DataPacket
{
    float time; // time from MCU
    float az, ay, ax; // accelerations
    float vx, vy, vz; // velosities
    float sx, sy, sz; // hz mb peremeshcenie
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
