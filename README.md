# Modbus Communication Project Overview

## Introduction

This project implements a comprehensive Modbus communication system, including master and slave functionalities. It simulates essential Modbus protocol operations such as reading and writing coils, discrete inputs, holding registers, and input registers, along with single and multiple write operations.

## Project Structure

The project comprises the following key components:

- **`slave.c`**: Implements the Modbus slave simulator, handling requests and generating appropriate responses.
- **`masterDll.c`**: Core functionalities for the Modbus master, including request sending and response processing.
- **`test_master.c`**: Test cases verifying various Modbus master operations.
- **`master_run.c`**: Demonstrates practical usage of the Modbus master DLL in an application.
- **`modbusUtil.h`**: Provides utility functions for Modbus operations, including CRC calculation and RTS control.

## Key Features

### Modbus Slave (`slave.c`)

The slave simulator listens and responds to Modbus requests based on specified function codes:

- **Read Coils (0x01) & Read Discrete Inputs (0x02)**: Generates dummy data for testing purposes.

* **Read Holding Registers (0x03) & Read Input Registers (0x04)**: Provides simulated register values based on requested addresses.
* **Write Single Coil/Register (0x05, 0x06) & Write Multiple Coils/Registers (0x0F, 0x10)**: Echoes back requests as confirmation responses.

### Modbus Master (`masterDll.c`)

The master DLL handles sending Modbus requests and processing responses, supporting:

- Reading operations: coils, discrete inputs, holding registers, input registers.
- Writing operations: single and multiple coils/registers.
- COM port and communication parameter configurations.

### Test Cases (`test_master.c`)

Test cases validate the master DLL functionality by communicating with the slave simulator, covering all supported Modbus operations and displaying results.

### Standalone Application (`master_run.c`)

This component showcases the practical use of the master DLL, sending read requests to the slave simulator and logging responses in CSV format for analysis.

## Technical Highlights

### Communication Configuration

Utilizes Windows API to configure communication settings, including:

- Baud rate
- Byte size
- Parity
- Stop bits
- Communication timeouts

### CRC Calculation

- Employs the `modbus_crc` function from `modbusUtil.h` to ensure data integrity through accurate CRC calculations for Modbus frames.

### RTS Control

- Implements half-duplex communication via the `setRTS` function, managing RTS signals appropriately.

### CSV Logging

- Features the `write_csv` function in the master DLL for systematic logging of responses, aiding debugging and analysis.

## How to Run

1. Compile the project using a C compiler (e.g., GCC or MSVC).
2. Connect master and slave devices to designated COM ports.
3. Execute the slave simulator (`slave.exe`) followed by the master test program (`test_master.exe`).
4. Observe outputs displayed in the console and logged to CSV files.
