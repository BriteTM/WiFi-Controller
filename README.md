# WiFi-Controller

Embedded C firmware for a pocket-sized **ESP32-C3** wireless controller and encounter counter built with **ESP-IDF** and **FreeRTOS** (via PlatformIO), designed to interface with modded Nintendo 3DS **Rosalina Input Redirection**.

## Overview
- **Rosalina Input Redirection Over UDP:** Configures the ESP32-C3 as a Wi-Fi SoftAP and streams low-latency UDP controller input packets directly to a modded 3DS.
- **Persistent Encounter Counter:** Tracks session/encounter counts on an integrated `64x32` `I2C` OLED display and saves counts to non-volatile flash memory (`NVS`) so progress persists across power cycles.
- **Concurrent FreeRTOS Architecture:** Separates debounced GPIO button polling, UDP packet transmission, flash writes, and OLED telemetry updates across FreeRTOS tasks in ESP-IDF.
- **Portable Hardware:** Housed in a custom 3D-printed enclosure powered by a single-cell LiPo battery with a `TP4056` USB charger.

## Repository Structure
- `src/` — Core application firmware (SoftAP/UDP networking, Rosalina packet formatting, NVS flash storage, GPIO polling, and OLED UI)
- `include/` — Project header files and pin/configuration definitions
- `lib/` — Custom or third-party component libraries
- `test/` — Hardware and unit test files
- `platformio.ini` & `CMakeLists.txt` — PlatformIO + ESP-IDF build configuration
- `.clang-format` — C code formatting rules

## Build & Flash
1. Open the repository in **PlatformIO** (or use PlatformIO Core CLI).
2. Connect the ESP32-C3 target over USB and build/upload:
   ```bash
   pio run --target upload
