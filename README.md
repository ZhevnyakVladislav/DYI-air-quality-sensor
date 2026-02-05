# Air Quality Sensor

This project creates an Air Quality Sensor device using the ESP Matter data model with an e-ink display dashboard showing temperature, humidity, and pressure readings from an indoor BME280 sensor.

See the [docs](https://docs.espressif.com/projects/esp-matter/en/latest/esp32/developing.html) for more information about building and flashing the firmware.

## Features

- **Indoor Environmental Monitoring**: Temperature, Humidity, and Pressure measurements via BME280 sensor
- **e-ink Display Dashboard**: Real-time data visualization on a 296x128 pixel display
- **Matter over Thread Support**: Full ESP Matter integration for smart home compatibility

## Hardware

- **MCU**: ESP32-C3 (primary) or ESP32/S3/C6 variants
- **Sensor**: BME26
- **Sensor**: BME280 (I2C interface)
- **Display**: 2.7" e-ink display (SPI interface)

## Setup

### 1. Environment Setup

Set the required environment variables:

```bash
export ESP_MATTER_PATH=/path/to/esp-matter
export ESP_IDF_PATH=/path/to/esp-idf
```

### 2. Building and Flashing

```bash
idf.py build flash monitor
```

### 3. Device Configuration

After flashing, the device will:

1. Initialize the BME280 sensor via I2C
2. Display startup information on the e-ink display
3. Begin measuring and displaying environmental data

## Project Structure

```
main/
  ├── main.cpp              - Main application entry point
  ├── WeatherSensor.cpp/.h  - BME280 sensor interface
  ├── EpdDriver.cpp/.h      - e-ink display driver
  ├── Graphics.cpp/.h       - Display graphics library
  ├── MatterNode.cpp/.h     - Matter device configuration
  ├── drivers/              - BME280 driver files
  ├── fonts/                - Display font files
  └── icons/                - Display icon definitions
```

## Device Performance

- **Memory**: ~105-108KB free internal RAM after commissioning
- **Flash Usage**: ~1.3MB firmware binary
- **Update Interval**: Display refreshes every 30 seconds
- **Sensor Sampling**: Continuous BME280 measurements
