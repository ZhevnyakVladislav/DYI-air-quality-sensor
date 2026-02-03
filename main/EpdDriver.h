#pragma once
#include "driver/spi_master.h"
#include "driver/gpio.h"

class EpdDriver {
public:
    EpdDriver(gpio_num_t mosi, gpio_num_t clk, gpio_num_t cs, gpio_num_t dc, gpio_num_t rst, gpio_num_t busy);
    void init();
    void wake(bool partial);
    void sleep();
    void display(const uint8_t* buffer, bool partial);

private:
    void reset();
    void sendCmd(uint8_t cmd);
    void sendData(uint8_t data);
    void waitBusy();
    
    spi_device_handle_t spi;
    gpio_num_t pin_cs, pin_dc, pin_rst, pin_busy;
};