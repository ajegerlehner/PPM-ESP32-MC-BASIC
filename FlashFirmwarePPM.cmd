esptool --chip esp32 --port COM8 --baud 460800 ^
    --before default_reset --after hard_reset ^
    write_flash -z ^
    --flash_mode dio --flash_size detect --flash_freq 40m ^
    0x1000   build\bootloader\bootloader.bin ^
    0x8000   build\partition_table\partition-table.bin ^
    0xD000   build\ota_data_initial.bin ^
    0x10000  build\ppm-esp32-mc-basic.bin ^
    0xB10000 build\spiffs.bin