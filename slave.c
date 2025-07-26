#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "modbusUtil.h" 

#define SLAVE_ID 1
#define MODBUS_MAX_FRAME 260

void send_response(HANDLE hCom, uint8_t *response, int len) {
    uint16_t crc = modbus_crc(response, len);
    response[len] = crc & 0xFF;
    response[len + 1] = crc >> 8;

    setRTS(hCom, TRUE);
    Sleep(2);
    DWORD written;
    WriteFile(hCom, response, len + 2, &written, NULL);
    FlushFileBuffers(hCom);
    Sleep(2);
    setRTS(hCom, FALSE);
}

// =================== Function Code Handlers ===================

// 0x01 & 0x02: Read Coils / Discrete Inputs
void respond_read_bits(HANDLE hCom, uint8_t func_code, uint16_t quantity) {
    uint8_t byte_count = (quantity + 7) / 8;
    uint8_t response[MODBUS_MAX_FRAME] = {0};
    response[0] = SLAVE_ID;
    response[1] = func_code;
    response[2] = byte_count;
    for (int i = 0; i < byte_count; i++) {
        response[3 + i] = (func_code == 0x01) ? 0xAA : 0x55; // Dummy data
    }
    send_response(hCom, response, 3 + byte_count);
}

// 0x03 & 0x04: Read Holding/Input Registers
void respond_read_registers(HANDLE hCom, uint8_t func_code, uint16_t start_addr, uint16_t quantity) {
    uint8_t response[MODBUS_MAX_FRAME] = {0};
    response[0] = SLAVE_ID;
    response[1] = func_code;
    response[2] = quantity * 2;
    for (int i = 0; i < quantity; i++) {
        response[3 + 2 * i] = 0x12;               // High byte (dummy)
        response[4 + 2 * i] = (uint8_t)(start_addr + i); // Low byte
    }
    send_response(hCom, response, 3 + 2 * quantity);
}

// 0x05 & 0x06: Write Single Coil/Register
void respond_write_single(HANDLE hCom, uint8_t *req) {
    uint8_t response[8];
    memcpy(response, req, 6);
    send_response(hCom, response, 6);
}

// 0x0F & 0x10: Write Multiple Coils/Registers
void respond_write_multiple(HANDLE hCom, uint8_t *req) {
    uint8_t response[8];
    memcpy(response, req, 6);
    send_response(hCom, response, 6);
}

// =================== Request Dispatcher ===================
void handle_request(HANDLE hCom, uint8_t *req, int len) {
    if (len < 8) return;

    // CRC Check
    uint16_t crc = modbus_crc(req, len - 2);
    if (req[len - 2] != (crc & 0xFF) || req[len - 1] != (crc >> 8)) {
        printf("CRC Error\n");
        return;
    }

    if (req[0] != SLAVE_ID) return; // Ignore other slave IDs

    uint8_t func = req[1];
    uint16_t start_addr = (req[2] << 8) | req[3];
    uint16_t quantity = (req[4] << 8) | req[5];

    printf("Received request: Function 0x%02X, Start Addr %d, Quantity %d\n", func, start_addr, quantity);

    switch (func) {
        case 0x01: respond_read_bits(hCom, func, quantity); break;
        case 0x02: respond_read_bits(hCom, func, quantity); break;
        case 0x03: respond_read_registers(hCom, func, start_addr, quantity); break;
        case 0x04: respond_read_registers(hCom, func, start_addr, quantity); break;
        case 0x05: respond_write_single(hCom, req); break;
        case 0x06: respond_write_single(hCom, req); break;
        case 0x0F: respond_write_multiple(hCom, req); break;
        case 0x10: respond_write_multiple(hCom, req); break;
        default:
            printf("Unknown Function 0x%02X\n", func);
            break;
    }
}

// =================== Main ===================
int main() {
    ModbusConfig config = {
        .port_name = "\\\\.\\COM10",
        .baud_rate = CBR_9600,
        .byte_size = 8,
        .parity = NOPARITY,
        .stop_bits = ONESTOPBIT,
        .read_interval = 50,
        .read_constant = 50,
        .read_multiplier = 10,
        .num_slaves = 1
    };

    HANDLE hCom;
    DCB dcb;
    COMMTIMEOUTS timeouts;

    hCom = CreateFileA(config.port_name, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                       OPEN_EXISTING, 0, NULL);
    if (hCom == INVALID_HANDLE_VALUE) {
        printf("Failed to open COM port\n");
        return 1;
    }

    GetCommState(hCom, &dcb);
    dcb.BaudRate = config.baud_rate;
    dcb.ByteSize = config.byte_size;
    dcb.Parity = config.parity;
    dcb.StopBits = config.stop_bits;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    SetCommState(hCom, &dcb);

    GetCommTimeouts(hCom, &timeouts);
    timeouts.ReadIntervalTimeout = config.read_interval;
    timeouts.ReadTotalTimeoutConstant = config.read_constant;
    timeouts.ReadTotalTimeoutMultiplier = config.read_multiplier;
    SetCommTimeouts(hCom, &timeouts);

    printf("Modbus Slave Simulator Started (Waiting for requests...)\n");

    while (1) {
        uint8_t buf[MODBUS_MAX_FRAME];
        DWORD received;
        if (ReadFile(hCom, buf, sizeof(buf), &received, NULL) && received > 0) {
            handle_request(hCom, buf, received);
        }
        Sleep(10);
    }

    CloseHandle(hCom);
    return 0;
}
