#include "EpdDriver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>

// The Magic LUT for fast partial refresh (SSD1680)
static const uint8_t lut_partial[] = {
    0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
    0x00, 0x00, 0x00, 0x22, 0x17, 0x41, 0xB0, 0x32, 0x36,
};

EpdDriver::EpdDriver(gpio_num_t mosi, gpio_num_t clk, gpio_num_t cs, gpio_num_t dc, gpio_num_t rst, gpio_num_t busy)
    : pin_cs(cs), pin_dc(dc), pin_rst(rst), pin_busy(busy) {
    
    // GPIO Config
    gpio_config_t io_conf = {};
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = ((1ULL<<cs)|(1ULL<<dc)|(1ULL<<rst));
    gpio_config(&io_conf);
    
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL<<busy);
    gpio_config(&io_conf);

    // SPI Config (Safe Order Fix)
    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num = mosi;
    buscfg.miso_io_num = -1;
    buscfg.sclk_io_num = clk;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = 4096;
    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t devcfg = {};
    devcfg.clock_speed_hz = 2000000;
    devcfg.mode = 0;
    devcfg.spics_io_num = -1;
    devcfg.queue_size = 7;
    spi_bus_add_device(SPI2_HOST, &devcfg, &spi);
}

void EpdDriver::reset() {
    gpio_set_level(pin_rst, 1); vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(pin_rst, 0); vTaskDelay(pdMS_TO_TICKS(5));
    gpio_set_level(pin_rst, 1); vTaskDelay(pdMS_TO_TICKS(20));
    waitBusy();
}

void EpdDriver::waitBusy() {
    vTaskDelay(pdMS_TO_TICKS(20));
    int timeout = 0;
    while (gpio_get_level(pin_busy) == 1) {
        vTaskDelay(pdMS_TO_TICKS(20));
        if (timeout++ > 100) break;
    }
}

void EpdDriver::sendCmd(uint8_t cmd) {
    gpio_set_level(pin_dc, 0); gpio_set_level(pin_cs, 0);
    spi_transaction_t t = { .length = 8, .tx_buffer = &cmd };
    spi_device_polling_transmit(spi, &t);
    gpio_set_level(pin_cs, 1);
}

void EpdDriver::sendData(uint8_t data) {
    gpio_set_level(pin_dc, 1); gpio_set_level(pin_cs, 0);
    spi_transaction_t t = { .length = 8, .tx_buffer = &data };
    spi_device_polling_transmit(spi, &t);
    gpio_set_level(pin_cs, 1);
}

void EpdDriver::wake(bool partial) {
    // FORCE FULL REFRESH FOR DEBUGGING
    partial = false; 

    reset();
    if (partial) {
        // ... (this will be skipped for now) ...
    }
    
    // Standard Full Init Sequence
    sendCmd(0x12); waitBusy();
    sendCmd(0x3C); sendData(0x05); // Border waveform
    sendCmd(0x11); sendData(0x03); // Data Entry Mode (X+, Y+)
    sendCmd(0x44); sendData(0x00); sendData(0x0F); // RAM X start/end (0..15)
    sendCmd(0x45); sendData(0x00); sendData(0x00); sendData(0x27); sendData(0x01); // RAM Y start/end (0..295)

    // --- ADD THESE TWO LINES TO FIX ALIGNMENT ---
    sendCmd(0x4E); sendData(0x00); // Set RAM X Cursor to 0
    sendCmd(0x4F); sendData(0x00); sendData(0x00); // Set RAM Y Cursor to 0
    // ---------------------------------------------

    sendCmd(0x21); sendData(0x00); sendData(0x80); // Display Update Control
    waitBusy();
}

void EpdDriver::display(const uint8_t* buffer, bool partial) {
    sendCmd(0x24);
    for (int i = 0; i < 4736; i++) sendData(buffer[i]);

    if (partial) {
        sendCmd(0x22); sendData(0xCF);
        sendCmd(0x20);
        vTaskDelay(pdMS_TO_TICKS(500));
    } else {
        sendCmd(0x22); sendData(0xF7);
        sendCmd(0x20);
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

void EpdDriver::sleep() {
    sendCmd(0x10); sendData(0x01);
}