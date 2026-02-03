#include "WeatherSensor.h"
#include "esp_log.h"
#include <cstring>

#define I2C_PORT I2C_NUM_0
#define BME_ADDR 0x77

// C-style wrappers for the BME library
static int8_t i2c_read(uint8_t reg, uint8_t *data, uint32_t len, void *) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BME_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BME_ADDR << 1) | I2C_MASTER_READ, true);
    if (len > 1) i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return (ret == ESP_OK) ? BME280_OK : BME280_E_COMM_FAIL;
}

static int8_t i2c_write(uint8_t reg, const uint8_t *data, uint32_t len, void *) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BME_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return (ret == ESP_OK) ? BME280_OK : BME280_E_COMM_FAIL;
}

static void delay_us(uint32_t p, void *) { vTaskDelay(pdMS_TO_TICKS(p/1000)); }

WeatherSensor::WeatherSensor(gpio_num_t sda, gpio_num_t scl) {
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = sda;
    conf.scl_io_num = scl;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 100000;
    i2c_param_config(I2C_PORT, &conf);
    i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);
}

void WeatherSensor::init() {
    static uint8_t addr = BME_ADDR;
    dev.intf = BME280_I2C_INTF;
    dev.read = i2c_read;
    dev.write = i2c_write;
    dev.delay_us = delay_us;
    dev.intf_ptr = &addr;

    bme280_init(&dev);
    
    struct bme280_settings s;
    bme280_get_sensor_settings(&s, &dev);
    s.osr_h = BME280_OVERSAMPLING_1X;
    s.osr_p = BME280_OVERSAMPLING_1X;
    s.osr_t = BME280_OVERSAMPLING_1X;
    s.filter = BME280_FILTER_COEFF_OFF;
    bme280_set_sensor_settings(BME280_SEL_ALL_SETTINGS, &s, &dev);
    bme280_set_sensor_mode(BME280_POWERMODE_NORMAL, &dev);
}

bool WeatherSensor::read(Data &out) {
    struct bme280_data d;
    if (bme280_get_sensor_data(BME280_ALL, &d, &dev) == BME280_OK) {
        out.temp = d.temperature;
        out.hum = d.humidity;
        out.pres = d.pressure / 100.0f;
        return true;
    }
    return false;
}