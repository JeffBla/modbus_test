#include <stdio.h>
#include <stdint.h>
#include <windows.h>

typedef struct {
    const char* port_name;
    DWORD baud_rate;
    BYTE byte_size;
    BYTE parity;
    BYTE stop_bits;
    DWORD read_interval;
    DWORD read_constant;
    DWORD read_multiplier;
    int num_slaves;
} MasterConfig;

__declspec(dllimport) int start_master(const MasterConfig* cfg);
__declspec(dllimport) int send_read_request(uint8_t slave_id, uint16_t start_addr, uint16_t quantity, uint8_t* out, int* out_len);
__declspec(dllimport) void stop_master();
__declspec(dllimport) void write_csv(const char* filename, int slaveId, const uint8_t* data, int len);

int main() {
    MasterConfig cfg = {
        .port_name = "\\\\.\\COM9", 
        .baud_rate = CBR_9600,
        .byte_size = 8,
        .parity = NOPARITY,
        .stop_bits = ONESTOPBIT,
        .read_interval = 50,
        .read_constant = 50,
        .read_multiplier = 10,
        .num_slaves = 1
    };

    // 啟動主站
    if (start_master(&cfg) != 0) {
        printf("Failed to start master.\n");
        return 1;
    }

    // 發送讀取請求
    uint8_t response[256];
    int resp_len = 0;
    uint8_t slave_id = 1;
    uint16_t start_addr = 0x0000;
    uint16_t quantity = 2;

    if (send_read_request(slave_id, start_addr, quantity, response, &resp_len) == 0) {
        printf("Slave %d responded %d bytes: ", slave_id, resp_len);
        for (int i = 0; i < resp_len; i++)
            printf("%02X ", response[i]);
        printf("\n");

        // 寫入 CSV
        write_csv("output.csv", slave_id, response, resp_len);
    } else {
        printf("No response or error.\n");
    }

    stop_master();
    return 0;
}
