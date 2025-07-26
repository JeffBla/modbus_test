#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#include "masterDll.h"
#include "modbusUtil.h"  

int main() {
    ModbusConfig cfg = {
        "\\\\.\\COM9",  // 注意 COM Port 需要加 "\\\\.\\" 前綴
        CBR_9600,
        8,
        NOPARITY,
        ONESTOPBIT,
        50, 100, 10,    // Timeout 設定
        1               // Slave 數量
    };

    if (start_master(&cfg) != 0) {
        printf("❌ Failed to start master\n");
        return 1;
    }
    printf("✅ Master started on %s\n", cfg.port_name);

    uint8_t buffer[256];
    int length;

    // === 測試 0x01: 讀取線圈 ===
    printf("\n[TEST] 0x01 Read Coils\n");
    if (send_read_coils(1, 0, 8, buffer, &length) == 0) {
        printf("Read Coils Response (%d bytes): ", length);
        for (int i = 0; i < length; ++i) printf("%02X ", buffer[i]);
        printf("\n");
        write_csv("test_log.csv", 1, buffer, length);
    } else printf("❌ Failed to read coils\n");

    // === 測試 0x02: 讀取離散輸入 ===
    printf("\n[TEST] 0x02 Read Discrete Inputs\n");
    if (send_read_discrete_inputs(1, 0, 8, buffer, &length) == 0) {
        printf("Read Discrete Inputs Response (%d bytes): ", length);
        for (int i = 0; i < length; ++i) printf("%02X ", buffer[i]);
        printf("\n");
    } else printf("❌ Failed to read discrete inputs\n");

    // === 測試 0x03: 讀取保持暫存器 ===
    printf("\n[TEST] 0x03 Read Holding Registers\n");
    if (send_read_registers(1, 0, 4, buffer, &length) == 0) {
        printf("Read Holding Registers Response (%d bytes): ", length);
        for (int i = 0; i < length; ++i) printf("%02X ", buffer[i]);
        printf("\n");
    } else printf("❌ Failed to read holding registers\n");

    // === 測試 0x04: 讀取輸入暫存器 ===
    printf("\n[TEST] 0x04 Read Input Registers\n");
    if (send_read_input_registers(1, 0, 4, buffer, &length) == 0) {
        printf("Read Input Registers Response (%d bytes): ", length);
        for (int i = 0; i < length; ++i) printf("%02X ", buffer[i]);
        printf("\n");
    } else printf("❌ Failed to read input registers\n");

    // === 測試 0x05: 單線圈寫入 ===
    printf("\n[TEST] 0x05 Write Single Coil\n");
    if (send_write_single_coil(1, 0, TRUE) == 0)
        printf("Write Single Coil OK\n");
    else printf("❌ Failed to write single coil\n");

    // === 測試 0x06: 單暫存器寫入 ===
    printf("\n[TEST] 0x06 Write Single Register\n");
    if (send_write_single_register(1, 0, 0x1234) == 0)
        printf("Write Single Register OK\n");
    else printf("❌ Failed to write single register\n");

    // === 測試 0x0F: 多線圈寫入 ===
    printf("\n[TEST] 0x0F Write Multiple Coils\n");
    uint8_t coil_data[1] = { 0xAA }; // 8 coils
    if (send_write_multiple_coils(1, 0, 8, coil_data, 1) == 0)
        printf("Write Multiple Coils OK\n");
    else printf("❌ Failed to write multiple coils\n");

    // === 測試 0x10: 多暫存器寫入 ===
    printf("\n[TEST] 0x10 Write Multiple Registers\n");
    uint16_t reg_values[2] = { 0x1111, 0x2222 };
    if (send_write_multiple_registers(1, 0, 2, reg_values) == 0)
        printf("Write Multiple Registers OK\n");
    else printf("❌ Failed to write multiple registers\n");

    stop_master();
    printf("\nMaster stopped.\n");
    return 0;
}
