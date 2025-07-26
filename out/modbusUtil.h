#pragma once

#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

typedef struct {
    const char* port_name;     // "\\\\.\\COM9"
    DWORD baud_rate;           // CBR_9600
    BYTE byte_size;            // usually 8
    BYTE parity;               // NOPARITY
    BYTE stop_bits;            // ONESTOPBIT
    DWORD read_interval;       // ms
    DWORD read_constant;       // ms
    DWORD read_multiplier;     // ms/byte
    int num_slaves;            // number of slaves
} ModbusConfig;

static void setRTS(HANDLE hCom, BOOL enable) {
    if (enable) EscapeCommFunction(hCom, SETRTS);
    else EscapeCommFunction(hCom, CLRRTS);
}

static uint16_t modbus_crc(uint8_t *buf, int len) {
    uint16_t crc = 0xFFFF;
    for (int pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)buf[pos];
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else
                crc >>= 1;
        }
    }
    return crc;
}
