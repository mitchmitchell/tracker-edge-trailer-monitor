/*
 * Copyright (c) 2020 Particle Industries, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Particle.h"
#include "tracker_config.h"
#include "tracker.h"
#include "environment.h"


SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

#if TRACKER_PRODUCT_NEEDED
PRODUCT_ID(TRACKER_PRODUCT_ID);
#endif // TRACKER_PRODUCT_NEEDED
PRODUCT_VERSION(TRACKER_PRODUCT_VERSION);

STARTUP(
    Tracker::startup();
);

SerialLogHandler logHandler(115200, LOG_LEVEL_TRACE, {
    { "app.gps.nmea", LOG_LEVEL_INFO },
    { "app.gps.ubx",  LOG_LEVEL_INFO },
    { "ncp.at", LOG_LEVEL_INFO },
    { "net.ppp.client", LOG_LEVEL_INFO },
});

void loc_gen_cb(JSONWriter &writer, LocationPoint &point, const void *context)
{
    extern TemperatureHumidityValidator Validator;
    
    writer.name("env_t").value(Validator.getTemperatureC());
    writer.name("env_h").value(Validator.getHumidity());
 
    int ps = System.powerSource();

    writer.name("pwr").value(ps);
}

bool powerState()
{
    static bool oldPowerState = true; // assume we boot up with power connected -- we'll complain as soon as we know different
    bool newPowerState = false;
    power_source_t ps = (power_source_t)System.powerSource();

    switch(ps) {
        case POWER_SOURCE_VIN:
        case POWER_SOURCE_USB_HOST:
        case POWER_SOURCE_USB_ADAPTER:
        case POWER_SOURCE_USB_OTG: {
            newPowerState = true;
            break;
        }
        
        case POWER_SOURCE_UNKNOWN:
        case POWER_SOURCE_BATTERY:
        default: {
            newPowerState = false;
            break;
        }

    }

    if (oldPowerState != newPowerState) {
        Tracker::instance().location.triggerLocPub(Trigger::IMMEDIATE,(oldPowerState == true ? "pwr_l" : "pwr_r"));
        oldPowerState = newPowerState;
    }
 
    return oldPowerState;
};

void envState()
{
    if (environment_high_temperature_events())
    {
        Tracker::instance().location.triggerLocPub(Trigger::NORMAL,"envtemp_h");
    }

    if (environment_low_temperature_events())
    {
        Tracker::instance().location.triggerLocPub(Trigger::NORMAL,"envtemp_l");
    }
    if (environment_high_humidity_events())
    {
        Tracker::instance().location.triggerLocPub(Trigger::NORMAL,"envhum_h");
    }

    if (environment_low_humidity_events())
    {
        Tracker::instance().location.triggerLocPub(Trigger::NORMAL,"envhum_l");
    }
}

void setup()
{
    Tracker::instance().init();

    // Register a location callback so we can add temperature and humidity information
    // to location publishes
    Tracker::instance().location.regLocGenCallback(loc_gen_cb);
    
    environment_init();
}

void loop()
{
    environment_tick();
    envState();
    powerState();
    Tracker::instance().loop();
}
