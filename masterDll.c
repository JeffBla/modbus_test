#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "modbusUtil.h"

#define EXPORT __declspec(dllexport)

static HANDLE hCom = INVALID_HANDLE_VALUE;

typedef struct {
    const char* port_name;     // e.g. "\\\\.\\COM9"
    DWORD baud_rate;           // e.g. CBR_9600
    BYTE byte_size;            // usually 8
    BYTE parity;               // NOPARITY, EVENPARITY, etc.
    BYTE stop_bits;            // ONESTOPBIT, TWOSTOPBITS
    DWORD read_interval;       // ms
    DWORD read_constant;       // ms
    DWORD read_multiplier;     // ms/byte
    int num_slaves;         // number of slaves
} MasterConfig;

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

EXPORT int start_master(const MasterConfig* cfg) {
    if (!cfg) return -1;

    hCom = CreateFileA(cfg->port_name, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                       OPEN_EXISTING, 0, NULL);
    if (hCom == INVALID_HANDLE_VALUE) return -2;

    DCB dcb = {0};
    if (!GetCommState(hCom, &dcb)) return -3;

    dcb.BaudRate = cfg->baud_rate;
    dcb.ByteSize = cfg->byte_size;
    dcb.Parity = cfg->parity;
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

EXPORT int send_read_request(uint8_t slave_id, uint16_t start_addr, uint16_t quantity, uint8_t *out, int *out_len) {
    if (hCom == INVALID_HANDLE_VALUE) return -1;

    uint8_t request[8];
    request[0] = slave_id;
    request[1] = 0x03;
    request[2] = (start_addr >> 8) & 0xFF;
    request[3] = start_addr & 0xFF;
    request[4] = (quantity >> 8) & 0xFF;
    request[5] = quantity & 0xFF;

    uint16_t crc = modbus_crc(request, 6);
    request[6] = crc & 0xFF;
    request[7] = crc >> 8;

    setRTS(hCom, TRUE);
    Sleep(2);

    DWORD written;
    WriteFile(hCom, request, 8, &written, NULL);
    FlushFileBuffers(hCom);
    Sleep(2);

    setRTS(hCom, FALSE);

    DWORD received;
    BOOL ok = ReadFile(hCom, out, 256, &received, NULL);
    if (!ok || received < 5) return -2;

    *out_len = received;
    return 0;
}

EXPORT void stop_master() {
    if (hCom != INVALID_HANDLE_VALUE) {
        CloseHandle(hCom);
        hCom = INVALID_HANDLE_VALUE;
    }
}

EXPORT void write_csv(const char* filename, int slaveId, const uint8_t* data, int len) {
    if (!filename || !data || len <= 0) return;

    FILE *csv = fopen(filename, "a");
    if (!csv) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        return;
    }

    // 若檔案為空，則寫入 header（用 ftell 判斷檔案是否為空）
    fseek(csv, 0, SEEK_END);
    if (ftell(csv) == 0) {
        fprintf(csv, "Timestamp,SlaveID,Bytes,Data(hex)\n");
    }

    // write data
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