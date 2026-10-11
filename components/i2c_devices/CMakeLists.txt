idf_component_register(
    SRCS "i2c_bus/i2c_bus.c"
         "others/ssd1306/ssd1306.c"
         "others/ssd1306/ssd1306_fonts.c"
    INCLUDE_DIRS "."
                 "i2c_bus/include"
                 "others/ssd1306/include"
    REQUIRES esp_driver_i2c esp_driver_gpio
)
