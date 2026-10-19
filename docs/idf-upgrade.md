# ESP-IDF 6.1 and Azure IoT upgrade (October 2026)

The firmware moved from ESP-IDF 4.2/4.4 with the 2019 Azure IoT C SDK to
ESP-IDF 6.1 with the Azure SDK for Embedded C. This note records what changed,
why, and what is still open. Branch: `upgrade/idf-6.1`.

## Versions

| | Before | After |
|---|---|---|
| ESP-IDF | 4.2.1 / 4.4.2 | 6.1.0 |
| Azure client | azure-iot-sdk-c 1.2.14 + esp-azure port (vendored) | Azure SDK for Embedded C 1.5.0 (git submodule) on the ESP-IDF MQTT client |
| I2C | legacy `driver/i2c.h` | `driver/i2c_master.h` |
| MQTT, cJSON, LAN87xx PHY | part of ESP-IDF | component registry, pinned in `main/idf_component.yml` |

## Building

Use the ESP-IDF 6.1 shell (desktop shortcut "IDF_v6.1_Powershell", or
`. "C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1"` in PowerShell),
then from the repo root:

```powershell
git submodule update --init   # first checkout only: Azure SDK
idf.py build
idf.py -p COM6 flash monitor
```

The first build downloads the registry components into `managed_components/`
(ignored by git); `dependencies.lock` pins the exact versions and is committed.

## Decisions and why

- **Azure SDK for Embedded C instead of azure-iot-sdk-c.** Microsoft points
  constrained devices to it, and the old esp-azure port layer would have had to
  be ported by hand. It only builds topics and SAS tokens; `esp-mqtt` does the
  transport. All IoT Hub code is in `main/mcshared/mc_azure_hub_client.c`.
- **SAS tokens** are signed with PSA Crypto (HMAC-SHA256, volatile key, nothing
  stored in NVS), valid 1 hour and renewed 10 minutes before expiry from
  `process_azure_msgs()`. IoT Hub drops the connection on expiry, so renewal
  means a short reconnect, as with the old SDK.
- **TLS server certificates are verified** for IoT Hub, the config service and
  OTA, using the ESP-IDF certificate bundle. Before, `CONFIG_ESP_TLS_INSECURE`
  skipped verification and the pinned Baltimore root had expired (May 2025).
  TLS 1.0/1.1 are gone.
- **Main task stack is 8 KB** (was 3.5 KB). `app_main` makes the HTTPS call to
  the config service; the verifying handshake overflowed the old stack.
- **Heap debug options are off** (heap tracing, per-task tracking, abort on
  allocation failure; poisoning reduced to light) and mbedTLS frees its TLS
  buffers when idle (`CONFIG_MBEDTLS_DYNAMIC_BUFFER`). With them on, the heap
  ran out during certificate verification and the device crashed at boot.
  Don't re-enable them in production builds.
- **Newlib is kept** as the C library (6.x defaults new projects to Picolibc),
  so telemetry number formatting is unchanged.
- **Telemetry JSON** still uses parson (`components/parson`), so payloads are
  byte-identical to before.
- **OTA runs in its own task** so the MQTT connection and the watchdog keep
  running during a download.
- **The I2C bus layer** (`components/i2c_devices/i2c_bus`) offers write, read and
  write-read calls; each device driver issues the same transfers on the wire
  as before.

## Behaviour changes visible from the cloud or the log

- Failed direct methods return status 404 (unknown method) or 500 (NVSCLEAR
  error) instead of -1.
- Messages queued while offline are capped at 64 KB.
- The device connection string and Wi-Fi password are no longer logged.
- At boot, each I2C chip variant that isn't fitted logs one
  `I2C transaction unexpected nack detected` line. This is expected.
- Any failed I2C transfer, including a timeout, is reported as `ESP_FAIL`.

## Open items

- `ssd1306.c` (`iot_ssd1306_init`) reconfigures GPIO 23, 18 and 5, which are
  also the Ethernet management pins and counter input 0. Kept as before;
  check on the bench whether it can go.
- `CONFIG_ESP_HTTPS_OTA_ALLOW_HTTP` is still on (carried over). Turn it off if
  firmware URLs are always HTTPS.
- Secrets are still in the repo: the storage account key in `AzBinUpload*.cmd`
  and the config-service secret in `sdkconfig`. Rotate and move them out.
- `mc_web_server.c` reads `content_len` bytes into a 150-byte buffer
  (overflow on a long POST).
- Old build output is still committed in the repo root.

## Bench checks before rollout

1. Ethernet and Wi-Fi both get an address; Ethernet is preferred.
2. The config-service HTTPS call succeeds (look for
   `Free heap before device config request` in the log).
3. Telemetry arrives; the connection survives the hourly SAS token renewal.
4. Direct methods respond; a twin firmware change triggers OTA.
5. Sensors, I/O expanders, OLED, buttons and gesture sensor work.
6. A device on the previous field firmware can OTA to the new image and boot.
