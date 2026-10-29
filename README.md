# Nadi Pulse 

A Zephyr-based firmware project for the Nordic nRF54L15 DK that reads photoplethysmography (PPG) data from three MAX30102 sensors and exposes the captured waveform data over Bluetooth Low Energy (BLE).

The project is designed to monitor pulse signals from the wrist area and classify readings for three channels:

- VAT
- PIT
- KAF

These values are combined and sent to a connected mobile application via a custom BLE GATT service.

## Features

- 3x MAX30102 pulse sensors connected through separate I2C interfaces
- Data acquisition from wrist-area PPG sensors
- BLE peripheral support with custom GATT service
- Read/Write/Notify characteristic for control and streaming
- Device configuration for nRF54L15 DK target
- OTA update support via BLE using SMP/MCUboot integration

## Hardware

- Target board: nRF54L15 DK (`nrf54l15dk/nrf54l15/cpuapp`)
- Sensors: MAX30102 x3
- Communication: I2C bus with separate aliases for each sensor
- Wireless communication: Bluetooth LE

## Project structure

- `CMakeLists.txt` – Zephyr application entry point
- `prj.conf` – project configuration for Bluetooth, I2C, logging, and OTA support
- `sysbuild.conf` – Zephyr sysbuild configuration
- `nrf54l15dk_nrf54l15_cpuapp.overlay` – board overlay and peripheral pin mapping
- `src/main.c` – main sensor acquisition logic and BLE interaction
- `src/ble.h` – BLE service, notifications, advertising, and connection handling
- `instruction_to_build.txt` – quick build guidance

## Firmware behavior

The firmware initializes three MAX30102 sensors over I2C and reads their RED/IR samples. The signal difference between successive samples is calculated and normalized to reduce noise. The application then combines the three sensor streams and sends the resulting data over BLE.

The project uses BLE commands to select which sensor channel to enable:

- `c` – VAT
- `b` – PIT
- `a` – KAF
- `d` – all sensors enabled
- `e` – disable all sensors

These commands are handled through the custom GATT characteristic.

## Build instructions

This project is built using the Zephyr RTOS / nRF Connect SDK toolchain.

### Required environment

- Zephyr SDK / nRF Connect SDK installed
- `west` command available in PATH
- A compatible terminal environment for Nordic firmware builds

### Build steps

1. Open the project in a Zephyr-enabled environment.
2. Select the board target:
   - `nrf54l15dk/nrf54l15/cpuapp`
3. Select `prj.conf` as the application configuration.
4. Select the device tree overlay file:
   - `nrf54l15dk_nrf54l15_cpuapp.overlay`
5. Enable sysbuild.
6. Build the firmware.

The repository also contains a quick instruction file:

## Bluetooth details

The firmware exposes a custom BLE service with a read/write/notify characteristic for app communication. The project enables peripheral mode and advertises the device as:

- `NADI_PULSE_AV`

It uses a custom UUID-based service for sensor control and notifications.


## Repository summary
This project implements a wrist-based multi-sensor PPG measurement system on the nRF54L15 DK platform, combining hardware input from three MAX30102 sensors with BLE-based communication for downstream app integration and wireless data transfer.
