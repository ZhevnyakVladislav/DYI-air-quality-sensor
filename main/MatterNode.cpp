#include "MatterNode.h"
#include <esp_matter.h>
#include <esp_matter_console.h>
#include <esp_matter_endpoint.h>
#include <app_priv.h>
#include <app/server/Server.h>

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include <platform/ESP32/OpenthreadLauncher.h>
#endif

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

// --- GLOBAL VARIABLES ---
float g_ha_outdoor_temp = 0.0f;
float g_ha_feels_like   = 0.0f;
int   g_ha_hour         = 12;
int   g_ha_minute       = 0;

// --- ENDPOINT IDs ---
static uint16_t ep_temp = 0, ep_hum = 0, ep_pres = 0;
static uint16_t ep_vir_out_temp = 0;   
static uint16_t ep_vir_feels_like = 0; 
static uint16_t ep_vir_clock = 0;      

// --- CALLBACK ---
static esp_err_t app_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data) {
    if (type == attribute::PRE_UPDATE) {
        
        // 1. OUTDOOR TEMP (Light Brightness)
        if (endpoint_id == ep_vir_out_temp && cluster_id == LevelControl::Id && attribute_id == LevelControl::Attributes::CurrentLevel::Id) {
            if (val->val.u8 > 0) g_ha_outdoor_temp = (float)val->val.u8 - 50.0f;
        }

        // 2. FEELS LIKE (Light Brightness)
        else if (endpoint_id == ep_vir_feels_like && cluster_id == LevelControl::Id && attribute_id == LevelControl::Attributes::CurrentLevel::Id) {
            if (val->val.u8 > 0) g_ha_feels_like = (float)val->val.u8 - 50.0f;
        }

        // 3. CLOCK (Universal Color Handler)
        else if (endpoint_id == ep_vir_clock && cluster_id == ColorControl::Id) {
            
            attribute_t *attr_x = attribute::get(endpoint_id, ColorControl::Id, ColorControl::Attributes::CurrentX::Id);
            attribute_t *attr_y = attribute::get(endpoint_id, ColorControl::Id, ColorControl::Attributes::CurrentY::Id);
            
            if (attr_x && attr_y) {
                esp_matter_attr_val_t val_x, val_y;
                attribute::get_val(attr_x, &val_x);
                attribute::get_val(attr_y, &val_y);

                // --- THE MATH FIX ---
                // We add +32768 (half of 65536) before dividing.
                // This forces standard rounding (13.99 becomes 14, 13.01 becomes 13).
                
                int raw_hour = (val_x.val.u16 * 24) + 32768; 
                int new_hour = raw_hour / 65536;

                int raw_min = (val_y.val.u16 * 60) + 32768;
                int new_min = raw_min / 65536;

                // Safety Clamp
                if (new_hour > 23) new_hour = 23;
                if (new_min > 59) new_min = 59;

                // Only log if changed
                if (new_hour != g_ha_hour || new_min != g_ha_minute) {
                    g_ha_hour = new_hour;
                    g_ha_minute = new_min;
                    ESP_LOGI("MATTER", "Clock Synced! Time: %02d:%02d", g_ha_hour, g_ha_minute);
                }
            }
        }
    }
    return ESP_OK;
}

static esp_err_t app_id_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id, uint8_t effect_variant, void *priv_data) { return ESP_OK; }
static void app_evt_cb(const ChipDeviceEvent *event, intptr_t arg) {}

void MatterNode::init() {
    node::config_t node_config;
    node_t *node = node::create(&node_config, app_attribute_update_cb, app_id_cb);
    
    // 1. Real Sensors
    temperature_sensor::config_t t_cfg;
    humidity_sensor::config_t h_cfg;
    pressure_sensor::config_t p_cfg;
    ep_temp = endpoint::get_id(temperature_sensor::create(node, &t_cfg, ENDPOINT_FLAG_NONE, nullptr));
    ep_hum  = endpoint::get_id(humidity_sensor::create(node, &h_cfg, ENDPOINT_FLAG_NONE, nullptr));
    ep_pres = endpoint::get_id(pressure_sensor::create(node, &p_cfg, ENDPOINT_FLAG_NONE, nullptr));

    // 2. Lights
    dimmable_light::config_t l1_cfg;
    l1_cfg.on_off.on_off = true; l1_cfg.level_control.current_level = 70;
    ep_vir_out_temp = endpoint::get_id(dimmable_light::create(node, &l1_cfg, ENDPOINT_FLAG_NONE, nullptr));

    dimmable_light::config_t l2_cfg;
    l2_cfg.on_off.on_off = true; l2_cfg.level_control.current_level = 70;
    ep_vir_feels_like = endpoint::get_id(dimmable_light::create(node, &l2_cfg, ENDPOINT_FLAG_NONE, nullptr));

    // 3. Clock Light
    extended_color_light::config_t l3_cfg;
    l3_cfg.on_off.on_off = true;
    l3_cfg.color_control.color_mode = 0; 
    ep_vir_clock = endpoint::get_id(extended_color_light::create(node, &l3_cfg, ENDPOINT_FLAG_NONE, nullptr));

    #if CHIP_DEVICE_CONFIG_ENABLE_THREAD
    esp_openthread_platform_config_t ot_cfg = {
        .radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
        .host_config  = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
        .port_config  = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
    };
    set_openthread_platform_config(&ot_cfg);
    #endif
    esp_matter::start(app_evt_cb);
}

void MatterNode::update(float temp, float hum, float pres) {
    chip::DeviceLayer::SystemLayer().ScheduleLambda([=]() {
        attribute_t *attr;
        esp_matter_attr_val_t val = esp_matter_invalid(NULL);
        
        attr = attribute::get(ep_temp, TemperatureMeasurement::Id, TemperatureMeasurement::Attributes::MeasuredValue::Id);
        attribute::get_val(attr, &val); val.val.i16 = (int16_t)(temp * 100);
        attribute::update(ep_temp, TemperatureMeasurement::Id, TemperatureMeasurement::Attributes::MeasuredValue::Id, &val);

        attr = attribute::get(ep_hum, RelativeHumidityMeasurement::Id, RelativeHumidityMeasurement::Attributes::MeasuredValue::Id);
        attribute::get_val(attr, &val); val.val.u16 = (uint16_t)(hum * 100);
        attribute::update(ep_hum, RelativeHumidityMeasurement::Id, RelativeHumidityMeasurement::Attributes::MeasuredValue::Id, &val);

        attr = attribute::get(ep_pres, PressureMeasurement::Id, PressureMeasurement::Attributes::MeasuredValue::Id);
        attribute::get_val(attr, &val); val.val.u32 = (uint32_t)pres;
        attribute::update(ep_pres, PressureMeasurement::Id, PressureMeasurement::Attributes::MeasuredValue::Id, &val);
    });
}