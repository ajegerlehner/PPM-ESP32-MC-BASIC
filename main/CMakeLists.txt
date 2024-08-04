set(COMPONENT_SRCS  "mc_io_expander_hal.c" 
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
                    )

set(COMPONENT_EMBED_TXTFILES "../cert/azure.cer")

list(APPEND COMPONENT_ADD_INCLUDEDIRS "." 
                                      "./mcshared"
                                      "include"
                                      "include/mcshared"
                                      "../components")

register_component()

spiffs_create_partition_image(spiffs ../html FLASH_IN_PROJECT)