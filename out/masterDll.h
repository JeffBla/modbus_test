#ifndef MODBUS_MASTER_H
#define MODBUS_MASTER_H

#include <windows.h>
#include <stdint.h>

#include "modbusUtil.h"

// Exported functions
int start_master(const ModbusConfig* cfg);
void stop_master();
int send_read_coils(uint8_t slave_id, uint16_t start_addr, uint16_t quantity, uint8_t *out, int *out_len);
int send_read_discrete_inputs(uint8_t slave_id, uint16_t start_addr, uint16_t quantity, uint8_t *out, int *out_len);
int send_read_registers(uint8_t slave_id, uint16_t start_addr, uint16_t quantity, uint8_t *out, int *out_len);
int send_read_input_registers(uint8_t slave_id, uint16_t start_addr, uint16_t quantity, uint8_t *out, int *out_len);
int send_write_single_coil(uint8_t slave_id, uint16_t addr, BOOL value);
int send_write_single_register(uint8_t slave_id, uint16_t addr, uint16_t value);
int send_write_multiple_coils(uint8_t slave_id, uint16_t start_addr, uint16_t quantity, const uint8_t *coil_data, int data_bytes);
int send_write_multiple_registers(uint8_t slave_id, uint16_t start_addr, uint16_t quantity, const uint16_t *values);
void write_csv(const char* filename, int slaveId, const uint8_t* data, int len);

#endif // MODBUS_MASTER_H
