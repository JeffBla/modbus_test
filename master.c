#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "modbusUtil.h"

#define NUM_SLAVES 3

typedef struct {
    const char* port_name;     // e.g. "\\\\.\\COM9"
    DWORD baud_rate;           // e.g. CBR_9600
    BYTE byte_size;            // usually 8
    BYTE parity;               // NOPARITY, EVENPARITY, etc.
    BYTE stop_bits;            // ONESTOPBIT, TWOSTOPBITS
    DWORD read_interval;       // ms
    DWORD read_constant;       // ms
    DWORD read_multiplier;     // ms/byte
} MasterConfig;

MasterConfig cfg = {
    .port_name = "COM9",
    .baud_rate = CBR_9600,
    .byte_size = 8,
    .parity = NOPARITY,
    .stop_bits = ONESTOPBIT,
    .read_interval = 50,
    .read_constant = 50,
    .read_multiplier = 10
};

int main() {
    HANDLE hCom;
    DCB dcb;
    COMMTIMEOUTS timeouts;

    FILE *csv = fopen("output.csv", "a");
    if (!csv) {
        printf("Failed to open output.csv\n");
        return 1;
    }
    fprintf(csv, "Timestamp,SlaveID,Bytes,Data(hex)\n"); // CSV header

    hCom = CreateFileA(cfg.port_name, GENERIC_READ | GENERIC_WRITE, 0, 0,
        OPEN_EXISTING, 0, 0);
    if (hCom == INVALID_HANDLE_VALUE) {
        printf("Error opening COM port\n");
        return 1;
    }

    // 設定參數
    GetCommState(hCom, &dcb);
    dcb.BaudRate = cfg.baud_rate;
    dcb.ByteSize = cfg.byte_size;
    dcb.Parity = cfg.parity;
    dcb.StopBits = cfg.stop_bits;
    dcb.fRtsControl = RTS_CONTROL_DISABLE; // 我們手動控制
    SetCommState(hCom, &dcb);

    // 設定逾時
    GetCommTimeouts(hCom, &timeouts);
    timeouts.ReadIntervalTimeout = cfg.read_interval;
    timeouts.ReadTotalTimeoutConstant = cfg.read_constant;
    timeouts.ReadTotalTimeoutMultiplier = cfg.read_multiplier;
    SetCommTimeouts(hCom, &timeouts);

    while(1){
        for (int slave = 1; slave <= NUM_SLAVES; slave++) {
            uint8_t request[8] = {
                (uint8_t)slave, 0x03,        // slave ID + Function code
                0x00, 0x00,                  // Start Addr: 0x0000
                0x00, 0x02,                  // Quantity: 2 registers
                0x00, 0x00                   // CRC16 placeholder
            };

            // 計算 CRC
            uint16_t crc = modbus_crc(request, 6);
            request[6] = crc & 0xFF;        // low byte
            request[7] = crc >> 8;          // high byte

            // 切換到傳送模式
            setRTS(hCom, TRUE);
            Sleep(2);

            DWORD written;
            WriteFile(hCom, request, 8, &written, NULL);
            FlushFileBuffers(hCom);
            Sleep(2); // 給點穩定時間

            setRTS(hCom, FALSE); // 切回接收

            uint8_t response[256];
            DWORD received;
            ReadFile(hCom, response, 256, &received, NULL);

            printf("Slave %d - Received %d bytes: ", slave, received);
            for (DWORD i = 0; i < received; ++i)
                printf("%02X ", response[i]);
            printf("\n");

            // 寫入 CSV
            time_t now = time(NULL);
            struct tm *t = localtime(&now);
            char timestamp[32];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
            fprintf(csv, "\"%s\",%d,%d,\"", timestamp, slave, received);
            for (DWORD i = 0; i < received; ++i)
                fprintf(csv, "%02X ", response[i]);
            fprintf(csv, "\"\n");
            fflush(csv);

            Sleep(200);
        }
    }

    CloseHandle(hCom);
    fclose(csv);
    return 0;
}
