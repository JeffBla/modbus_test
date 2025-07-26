#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

#include "modbusUtil.h"

#define EXPORT __declspec(dllexport)
#define MODBUS_MAX_FRAME 260

static HANDLE hCom = INVALID_HANDLE_VALUE;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    return TRUE;
}

static int modbus_send_request(const uint8_t *req, int req_len, uint8_t *out, int *out_len) {
    if (hCom == INVALID_HANDLE_VALUE) return -1;

    setRTS(hCom, TRUE);
    Sleep(2);

    DWORD written;
    if (!WriteFile(hCom, req, req_len, &written, NULL) || written != req_len)
        return -2;

    FlushFileBuffers(hCom);
    Sleep(2);

    setRTS(hCom, FALSE);

    DWORD received;
    BOOL ok = ReadFile(hCom, out, MODBUS_MAX_FRAME, &received, NULL);
    if (!ok || received < 5) return -3;

    *out_len = (int)received;
    return 0;
}

static int modbus_build_read(uint8_t slave_id, uint8_t func_code, uint16_t start_addr, uint16_t quantity, uint8_t *req) {
    req[0] = slave_id;
    req[1] = func_code;
    req[2] = (start_addr >> 8) & 0xFF;
    req[3] = start_addr & 0xFF;
    req[4] = (quantity >> 8) & 0xFF;
    req[5] = quantity & 0xFF;
    uint16_t crc = modbus_crc(req, 6);
    req[6] = crc & 0xFF;
    req[7] = crc >> 8;
    return 8;
}

EXPORT int start_master(const ModbusConfig* cfg) {
    if (!cfg) return -1;
    hCom = CreateFileA(cfg->port_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hCom == INVALID_HANDLE_VALUE) return -2;

    DCB dcb = {0};
    if (!GetCommState(hCom, &dcb)) return -3;

    dcb.BaudRate = cfg->baud_rate;
    dcb.ByteSize = cfg->byte_size;
    dcb.Parity   = cfg->parity;
    dcb.StopBits = cfg->stop_bits;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;

    if (!SetCommState(hCom, &dcb)) return -4;

    COMMTIMEOUTS timeouts = {0};
    if (!GetCommTimeouts(hCom, &timeouts)) return -5;

    timeouts.ReadIntervalTimeout = cfg->read_interval;
    timeouts.ReadTotalTimeoutConstant = cfg->read_constant;
    timeouts.ReadTotalTimeoutMultiplier = cfg->read_multiplier;

    if (!SetCommTimeouts(hCom, &timeouts)) return -6;

    return 0;
}

EXPORT void stop_master() {
    if (hCom != INVALID_HANDLE_VALUE) {
        CloseHandle(hCom);
        hCom = INVALID_HANDLE_VALUE;
    }
}

// ===== Implement function code =====

// 0x01: Read Coils
EXPORT int send_read_coils(uint8_t slave_id, uint16_t start_addr, uint16_t quantity, uint8_t *out, int *out_len) {
    uint8_t req[8];
    int req_len = modbus_build_read(slave_id, 0x01, start_addr, quantity, req);
    return modbus_send_request(req, req_len, out, out_len);
}

// 0x02: Read Discrete Inputs
EXPORT int send_read_discrete_inputs(uint8_t slave_id, uint16_t start_addr, uint16_t quantity, uint8_t *out, int *out_len) {
    uint8_t req[8];
    int req_len = modbus_build_read(slave_id, 0x02, start_addr, quantity, req);
    return modbus_send_request(req, req_len, out, out_len);
}

// 0x03: Read Holding Registers
EXPORT int send_read_registers(uint8_t slave_id, uint16_t start_addr, uint16_t quantity, uint8_t *out, int *out_len) {
    uint8_t req[8];
    int req_len = modbus_build_read(slave_id, 0x03, start_addr, quantity, req);
    return modbus_send_request(req, req_len, out, out_len);
}

// 0x04: Read Input Registers
EXPORT int send_read_input_registers(uint8_t slave_id, uint16_t start_addr, uint16_t quantity, uint8_t *out, int *out_len) {
    uint8_t req[8];
    int req_len = modbus_build_read(slave_id, 0x04, start_addr, quantity, req);
    return modbus_send_request(req, req_len, out, out_len);
}

// 0x05: Write Single Coil
EXPORT int send_write_single_coil(uint8_t slave_id, uint16_t addr, BOOL value) {
    uint8_t req[8];
    req[0] = slave_id;
    req[1] = 0x05;
    req[2] = (addr >> 8) & 0xFF;
    req[3] = addr & 0xFF;
    req[4] = value ? 0xFF : 0x00;
    req[5] = 0x00;
    uint16_t crc = modbus_crc(req, 6);
    req[6] = crc & 0xFF;
    req[7] = crc >> 8;

    uint8_t resp[MODBUS_MAX_FRAME];
    int resp_len;
    return modbus_send_request(req, 8, resp, &resp_len);
}

// 0x06: Write Single Register
EXPORT int send_write_single_register(uint8_t slave_id, uint16_t addr, uint16_t value) {
    uint8_t req[8];
    req[0] = slave_id;
    req[1] = 0x06;
    req[2] = (addr >> 8) & 0xFF;
    req[3] = addr & 0xFF;
    req[4] = (value >> 8) & 0xFF;
    req[5] = value & 0xFF;
    uint16_t crc = modbus_crc(req, 6);
    req[6] = crc & 0xFF;
    req[7] = crc >> 8;

    uint8_t resp[MODBUS_MAX_FRAME];
    int resp_len;
    return modbus_send_request(req, 8, resp, &resp_len);
}

// 0x0F: Write Multiple Coils
EXPORT int send_write_multiple_coils(uint8_t slave_id, uint16_t start_addr, uint16_t quantity, const uint8_t *coil_data, int data_bytes) {
    if (!coil_data || data_bytes <= 0) return -1;

    uint8_t req[MODBUS_MAX_FRAME];
    req[0] = slave_id;
    req[1] = 0x0F;
    req[2] = (start_addr >> 8) & 0xFF;
    req[3] = start_addr & 0xFF;
    req[4] = (quantity >> 8) & 0xFF;
    req[5] = quantity & 0xFF;
    req[6] = (uint8_t)data_bytes;
    memcpy(&req[7], coil_data, data_bytes);

    uint16_t crc = modbus_crc(req, 7 + data_bytes);
    req[7 + data_bytes] = crc & 0xFF;
    req[8 + data_bytes] = crc >> 8;

    uint8_t response[MODBUS_MAX_FRAME];
    int out_len;
    return modbus_send_request(req, 9 + data_bytes, response, &out_len);
}

// 0x10: Write Multiple Registers
EXPORT int send_write_multiple_registers(uint8_t slave_id, uint16_t start_addr, uint16_t quantity, const uint16_t *values) {
    if (!values || quantity <= 0) return -1;

    uint8_t req[MODBUS_MAX_FRAME];
    req[0] = slave_id;
    req[1] = 0x10;
    req[2] = (start_addr >> 8) & 0xFF;
    req[3] = start_addr & 0xFF;
    req[4] = (quantity >> 8) & 0xFF;
    req[5] = quantity & 0xFF;
    req[6] = (uint8_t)(quantity * 2);

    for (int i = 0; i < quantity; ++i) {
        req[7 + i*2] = (values[i] >> 8) & 0xFF;
        req[8 + i*2] = values[i] & 0xFF;
    }

    int byte_count = 7 + quantity * 2;
    uint16_t crc = modbus_crc(req, byte_count);
    req[byte_count] = crc & 0xFF;
    req[byte_count + 1] = crc >> 8;

    uint8_t response[MODBUS_MAX_FRAME];
    int out_len;
    return modbus_send_request(req, byte_count + 2, response, &out_len);
}

// CSV record
EXPORT void write_csv(const char* filename, int slaveId, const uint8_t* data, int len) {
    if (!filename || !data || len <= 0) return;

    FILE *csv = fopen(filename, "a");
    if (!csv) return;

    fseek(csv, 0, SEEK_END);
    if (ftell(csv) == 0) fprintf(csv, "Timestamp,SlaveID,Bytes,Data(hex)\n");

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

    fprintf(csv, "\"%s\",%d,%d,\"", timestamp, slaveId, len);
    for (int i = 0; i < len; ++i)
        fprintf(csv, "%02X%s", data[i], (i < len - 1 ? " " : ""));
    fprintf(csv, "\"\n");

    fclose(csv);
}
