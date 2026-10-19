# PPM-ESP32-MC-BASIC

ESP32 machine-counter firmware: reads counters and sensors, sends telemetry to
Azure IoT Hub, receives direct methods and OTA updates via the device twin.

## Build

ESP-IDF 6.1 (`C:\esp\v6.1\esp-idf`). In PowerShell:
`. "C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1"`, then
`idf.py build` from the repo root. The Azure SDK is a git submodule
(`git submodule update --init`). MQTT, cJSON and the LAN87xx PHY come from the
component registry (`main/idf_component.yml`).

## Layout

- `main/` application; `main/mcshared/mc_azure_hub_client.c` is the IoT Hub client
- `components/azure-sdk-for-c/` Azure SDK for Embedded C (submodule + CMake wrapper)
- `components/i2c_devices/` I2C bus layer and OLED driver
- `components/parson/` JSON library used for telemetry payloads

## Notes

- Read `docs/idf-upgrade.md` before changing TLS, memory or `sdkconfig`
  settings; it explains the 6.1 upgrade decisions and known open items.
- Source files have mixed line endings (some CRLF); edit them without
  converting the whole file.
