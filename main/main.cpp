#include "EpdDriver.h"
#include "Graphics.h"
#include "WeatherSensor.h"
#include "MatterNode.h"
#include "Icons.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <cstdio>
#include <cstring>

static const char *TAG = "DASHBOARD";
WeatherSensor::Data g_last_data = {21.0f, 50.0f, 1013.0f};

// --- UI Layout ---
void draw_dashboard(Graphics &gfx, WeatherSensor::Data &indoor)
{

    gfx.clear();
    gfx.drawFastVLine(148, 5, 118, 0);
    gfx.drawRect(0, 0, 296, 128, 0);

    int left_column = 5;
    int right_column = 153;

    char buf[32];
    // --- LEFT: OUTSIDE ---
    gfx.drawString(left_column, 5, "Outside", &Font16, 0);

    // 1. Temp Number
    gfx.drawBitmap(left_column, 30, ICON_TEMPERATURE, 32, 32, 0);
    sprintf(buf, "%.1f", g_ha_outdoor_temp);
    gfx.drawString(52, 36, buf, &Font20, 0);
    gfx.drawBitmap(122, 36, ICON_DEGREE, 8, 8, 0);
    gfx.drawString(128, 36, "C", &Font20, 0);

    gfx.drawBitmap(left_column, 72, ICON_TEMPERATURE_FEELS_LIKE, 32, 32, 0);
    sprintf(buf, "%.1f", g_ha_feels_like);
    gfx.drawString(52, 78, buf, &Font20, 0);
    gfx.drawBitmap(122, 78, ICON_DEGREE, 8, 8, 0);
    gfx.drawString(128, 78, "C", &Font20, 0);

    // --- RIGHT: INSIDE ---
    gfx.drawString(right_column, 5, "Inside", &Font16, 0);

    gfx.drawBitmap(right_column, 30, ICON_TEMPERATURE, 32, 32, 0);
    sprintf(buf, "%.1f", indoor.temp);
    gfx.drawString(right_column + 52, 36, buf, &Font20, 0);
    gfx.drawBitmap(right_column + 118, 36, ICON_DEGREE, 8, 8, 0);
    gfx.drawString(right_column + 124, 36, "C", &Font20, 0);

    gfx.drawBitmap(right_column, 72, ICON_HUMIDITY, 32, 32, 0);
    sprintf(buf, "%.1f", indoor.hum);
    gfx.drawString(right_column + 52, 78, buf, &Font20, 0);
    gfx.drawString(right_column + 124, 78, "%", &Font20, 0);

    sprintf(buf, "%.0f hPa", indoor.pres);
    gfx.drawString(230, 111, buf, &Font12, 0);

    // --- CLOCK ---
    gfx.drawRect(230, 0, 66, 26, 0);
    char time_str[8];
    sprintf(time_str, "%02d:%02d", g_ha_hour, g_ha_minute);
    gfx.drawString(235, 5, time_str, &Font16, 0);
}

extern "C" void app_main()
{
    nvs_flash_init();

    EpdDriver display(GPIO_NUM_23, GPIO_NUM_18, GPIO_NUM_15, GPIO_NUM_22, GPIO_NUM_21, GPIO_NUM_20);
    Graphics gfx(296, 128);
    WeatherSensor sensor(GPIO_NUM_6, GPIO_NUM_7);

    MatterNode::init();
    sensor.init();

    ESP_LOGI(TAG, "Dashboard Started. Waiting 5s for sensor warmup...");
    vTaskDelay(pdMS_TO_TICKS(5000));

    while (true)
    {
        // 1. Read Indoor Sensor
        if (sensor.read(g_last_data))
        {
            ESP_LOGI(TAG, "SUCCESS! Temp: %.2f Hum: %.2f Pres: %.2f", g_last_data.temp, g_last_data.hum, g_last_data.pres);
            MatterNode::update(g_last_data.temp, g_last_data.hum, g_last_data.pres);
        }
        else
        {
            ESP_LOGE(TAG, "FAILED to read BME280 Sensor! Check Wiring!");
        }

        // 2. Draw Dashboard
        draw_dashboard(gfx, g_last_data);

        // 3. Refresh Screen
        display.wake(false);
        display.display(gfx.getBuffer(), false);
        display.sleep();

        ESP_LOGI(TAG, "Screen Updated. Sleeping 60s...");

        // 4. Strict 60-second Sleep
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}