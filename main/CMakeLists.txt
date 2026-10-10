idf_component_register(
    SRCS "mc_io_expander_hal.c"
         "mc_pca9554a.c"
         "main.c"
         "mc_mcp23008.c"
         "mc_sensor_data.c"
         "mc_esp32counter.c"
         "mc_esp32_gpio.c"
         "mcshared/mc_si7021.c"
         "mcshared/mc_vl6081x.c"
         "mcshared/mc_azure_hub_client.c"
         "mcshared/mc_common.c"
         "mcshared/mc_data.c"
         "mcshared/mc_oled_display.c"
         "mcshared/mc_iotcentral.c"
         "mcshared/mc_device.c"
         "mcshared/mc_gpio.c"
         "mcshared/mc_i2c_bus.c"
         "mcshared/bme280.c"
         "mcshared/mc_eth.c"
         "mcshared/mc_ota.c"
         "mcshared/mc_web_server.c"
         "mcshared/mc_mcp23017.c"
    INCLUDE_DIRS "."
                 "./mcshared"
                 "include"
                 "include/mcshared"
    PRIV_REQUIRES azure-sdk-for-c parson i2c_devices
                  app_update driver esp-tls esp_app_format esp_eth esp_event
                  esp_http_client esp_http_server esp_https_ota esp_netif
                  esp_timer esp_wifi json lwip mbedtls mqtt nvs_flash spiffs
)

spiffs_create_partition_image(spiffs ../html FLASH_IN_PROJECT)
