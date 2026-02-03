#pragma once
#include <cstdint>

// Exposed Data from HA
extern float g_ha_outdoor_temp;
extern float g_ha_feels_like;
extern int   g_ha_hour;
extern int   g_ha_minute;

namespace MatterNode {
    void init();
    void update(float temp, float hum, float pres);
}