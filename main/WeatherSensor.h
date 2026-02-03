#pragma once
#include "driver/i2c.h"
#include "bme280.h"

class WeatherSensor {
public:
    struct Data {
        float temp;
        float hum;
        float pres;
    };

    WeatherSensor(gpio_num_t sda, gpio_num_t scl);
    void init();
    bool read(Data &outData);

private:
    struct bme280_dev dev;
};