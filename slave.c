#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#define SLAVE_ID 1

void setRTS(HANDLE hCom, BOOL enable) {
    if (enable) EscapeCommFunction(hCom, SETRTS);
    else EscapeCommFunction(hCom, CLRRTS);
}

uint16_t modbus_crc(uint8_t *buf, int len) {
    uint16_t crc = 0xFFFF;
    for (int pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)buf[pos];
        for (int i = 0; i < 8; i++) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else
                crc >>= 1;
        }
    }
    return crc;
}

void respond_read_holding(HANDLE hCom, uint8_t *request) {
    uint16_t start_addr = (request[2] << 8) | request[3];
    uint16_t quantity = (request[4] << 8) | request[5];

    uint8_t response[256] = {0};
    response[0] = SLAVE_ID;
    response[1] = 0x03;
    response[2] = quantity * 2;  // byte count

    for (int i = 0; i < quantity; i++) {
        // 模擬暫存器內容（高位在前）
        response[3 + 2 * i] = 0x00;
        response[4 + 2 * i] = (uint8_t)(start_addr + i);  // 假裝每個暫存器值就是位址
    }

    uint16_t crc = modbus_crc(response, 3 + 2 * quantity);
    response[3 + 2 * quantity] = crc & 0xFF;
    response[4 + 2 * quantity] = crc >> 8;

    setRTS(hCom, TRUE);
    Sleep(2);  // 等方向穩定

    DWORD written;
    WriteFile(hCom, response, 5 + 2 * quantity, &written, NULL);
    FlushFileBuffers(hCom);
    Sleep(2); // 保留時間送完

    setRTS(hCom, FALSE);
}

int main() {
    HANDLE hCom;
    DCB dcb;
    COMMTIMEOUTS timeouts;

    hCom = CreateFileA("\\\\.\\COM10", GENERIC_READ | GENERIC_WRITE, 0, NULL,
                       OPEN_EXISTING, 0, NULL);
    if (hCom == INVALID_HANDLE_VALUE) {
        printf("無法開啟 COM\n");
        return 1;
    }

    GetCommState(hCom, &dcb);
    dcb.BaudRate = CBR_9600;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;  // 我們手動控制 RTS
    SetCommState(hCom, &dcb);

    GetCommTimeouts(hCom, &timeouts);
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    SetCommTimeouts(hCom, &timeouts);

    printf("模擬從站開始（等待請求...）\n");

    while (1) {
        uint8_t buf[256];
        DWORD received;
        ReadFile(hCom, buf, 8, &received, NULL);

        if (received >= 8) {
            uint16_t crc = modbus_crc(buf, 6);
            if ((buf[6] == (crc & 0xFF)) && (buf[7] == (crc >> 8))) {
                if ((buf[0] >= 1 && buf[0] <= 3) && buf[1] == 0x03) { // simulate slave ID from 1 to 3
                    printf("Slave %d - Received request: ", buf[0]);
                    respond_read_holding(hCom, buf);
                }
            } else {
                printf("CRC Error\n");
            }
        }

        Sleep(10);
    }

    CloseHandle(hCom);
    return 0;
}
