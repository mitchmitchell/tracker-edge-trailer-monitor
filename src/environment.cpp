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

#include <atomic>
#include "environment.h"
#include "tracker_sleep.h"


// Configuration based on external temperature and humidity sensor
SensorConfig _sensorConfig = {
  .minTemperature       = -40.0,        // minimum temperature to represent
  .maxTemperature       = 150.0,        // maximum temperature to represent
  .minHumidity          = 0.0,          // minimum humidity to represent
  .maxHumidity          = 100.0         // maximum humidity to represent
};


// Basic structure to hold all configuration fields
struct ConfigData {
  double highThreshold;
  bool highEnable;
  bool highLatch;
  double lowThreshold;
  bool lowEnable;
  bool lowLatch;
  double hysteresis;
  double humhighThreshold;
  bool humhighEnable;
  bool humhighLatch;
  double humlowThreshold;
  bool humlowEnable;
  bool humlowLatch;
  double humhysteresis;
};


static ConfigData _environmentConfig = {
    .highThreshold = ExternalTemperatureHighDefault,
    .highEnable = false,
    .highLatch = true,
    .lowThreshold = ExternalTemperatureLowDefault,
    .lowEnable = false,
    .lowLatch = true,
    .hysteresis = ExternalTemperatureHysteresisDefault,
    .humhighThreshold = ExternalHumidityHighDefault,
    .humhighEnable = false,
    .humhighLatch = true,
    .humlowThreshold = ExternalHumidityLowDefault,
    .humlowEnable = false,
    .humlowLatch = true,
    .humhysteresis = ExternalHumidityHysteresisDefault
};


// Configuration service node setup
// { "env_trig" :
//     { "high": 25.0,
//       "high_en": false,
//       "high_latch": true,
//       "low": 25.0,
//       "low_en": false
//       "low_latch": true,
//       "hyst": 5.0
//      }
//  }

enum class EnvState {
  UNKNOWN,        //< Initial state
  NORMAL,         //< Value is not outside limit and isn't pending a pass through the hysteresis limit
  OUTSIDE_LIMIT,  //< Value is outside of the given limit
  INSIDE_LIMIT,   //< Value is inside of the give limit and is pending a pass through the hysteresis limit
};

int environment_init() {

  static ConfigObject _envConfigObject
  (
      "env_trig",
      {
        ConfigFloat("envhigh", &_environmentConfig.highThreshold, _sensorConfig.minTemperature, _sensorConfig.maxTemperature),
        ConfigBool("envhigh_en", &_environmentConfig.highEnable),
        ConfigBool("envhigh_latch", &_environmentConfig.highLatch),
        ConfigFloat("envlow", &_environmentConfig.lowThreshold, _sensorConfig.minTemperature, _sensorConfig.maxTemperature),
        ConfigBool("envlow_en", &_environmentConfig.lowEnable),
        ConfigBool("envlow_latch", &_environmentConfig.lowLatch),
        ConfigFloat("envhyst", &_environmentConfig.hysteresis, 0.0, _sensorConfig.maxTemperature - _sensorConfig.minTemperature),
        ConfigFloat("humhigh", &_environmentConfig.humhighThreshold, _sensorConfig.minHumidity, _sensorConfig.maxHumidity),
        ConfigBool("humhigh_en", &_environmentConfig.humhighEnable),
        ConfigBool("humhigh_latch", &_environmentConfig.humhighLatch),
        ConfigFloat("humlow", &_environmentConfig.humlowThreshold, _sensorConfig.minHumidity, _sensorConfig.maxHumidity),
        ConfigBool("humlow_en", &_environmentConfig.humlowEnable),
        ConfigBool("humlow_latch", &_environmentConfig.humlowLatch),
        ConfigFloat("humhyst", &_environmentConfig.humhysteresis, 0.0, _sensorConfig.maxHumidity - _sensorConfig.minHumidity)
      }
  );

  CHECK(ConfigService::instance().registerModule(_envConfigObject));

  return SYSTEM_ERROR_NONE;
}

// All state and variables related to high threshold evaluation
static EnvState highState = EnvState::NORMAL;
static std::atomic<size_t> highEvents(0);
static size_t highEventsLast = 0;
static bool highLatch = false;
static EnvState humhighState = EnvState::NORMAL;
static std::atomic<size_t> humhighEvents(0);
static size_t humhighEventsLast = 0;
static bool humhighLatch = false;

size_t environment_high_temperature_events() {
  auto eventsCapture = highEvents.load();
  auto eventsCount = eventsCapture - highEventsLast;
  highEventsLast = eventsCapture;
  return (_environmentConfig.highLatch) ? highLatch : eventsCount;
}

size_t environment_high_humidity_events() {
  auto eventsCapture = humhighEvents.load();
  auto eventsCount = eventsCapture - humhighEventsLast;
  humhighEventsLast = eventsCapture;
  return (_environmentConfig.humhighLatch) ? humhighLatch : eventsCount;
}

// All state and variables related to low threshold evaluation
static EnvState lowState = EnvState::NORMAL;
static std::atomic<size_t> lowEvents(0);
static size_t lowEventsLast = 0;
static bool lowLatch = false;
static EnvState humlowState = EnvState::NORMAL;
static std::atomic<size_t> humlowEvents(0);
static size_t humlowEventsLast = 0;
static bool humlowLatch = false;

size_t environment_low_temperature_events() {
  auto eventsCapture = lowEvents.load();
  auto eventsCount = eventsCapture - lowEventsLast;
  lowEventsLast = eventsCapture;
  return (_environmentConfig.lowLatch) ? lowLatch : eventsCount;
}

size_t environment_low_humidity_events() {
  auto eventsCapture = humlowEvents.load();
  auto eventsCount = eventsCapture - humlowEventsLast;
  humlowEventsLast = eventsCapture;
  return (_environmentConfig.humlowLatch) ? humlowLatch : eventsCount;
}

void evaluate_user_environment(Environment environment) {
  // *** TEMPERATURE ***
  // Evaluate environment against high threshold
  if (_environmentConfig.highEnable) {
    switch (highState) {
      case EnvState::UNKNOWN:
        highState = EnvState::NORMAL;
        // Fall through
      case EnvState::NORMAL: {
        if (environment.Temperature >= _environmentConfig.highThreshold) {
          highEvents++;
          highLatch = true;
          highState = EnvState::OUTSIDE_LIMIT;
        }
        break;
      }

      case EnvState::OUTSIDE_LIMIT: {
        if (environment.Temperature < _environmentConfig.highThreshold) {
          highState = EnvState::INSIDE_LIMIT;
        }
        break;
      }

      case EnvState::INSIDE_LIMIT: {
        if (environment.Temperature <= _environmentConfig.highThreshold - _environmentConfig.hysteresis) {
          highLatch = false;
          highState = EnvState::NORMAL;
        }
        else if (environment.Temperature >= _environmentConfig.highThreshold) {
          highState = EnvState::OUTSIDE_LIMIT;
        }
        break;
      }
    }
  }

  // Evaluate environment against low threshold
  if (_environmentConfig.lowEnable) {
    switch (lowState) {
      case EnvState::UNKNOWN:
        lowState = EnvState::NORMAL;
        // Fall through
      case EnvState::NORMAL: {
        if (environment.Temperature <= _environmentConfig.lowThreshold) {
          lowEvents++;
          lowLatch = true;
          lowState = EnvState::OUTSIDE_LIMIT;
        }
        break;
      }

      case EnvState::OUTSIDE_LIMIT: {
        if (environment.Temperature > _environmentConfig.lowThreshold) {
          lowState = EnvState::INSIDE_LIMIT;
        }
        break;
      }

      case EnvState::INSIDE_LIMIT: {
        if (environment.Temperature >= _environmentConfig.lowThreshold + _environmentConfig.hysteresis) {
          lowLatch = false;
          lowState = EnvState::NORMAL;
        }
        else if (environment.Temperature <= _environmentConfig.lowThreshold) {
          lowState = EnvState::OUTSIDE_LIMIT;
        }
        break;
      }
    }
  }
// *** HUMIDITY ***
    // Evaluate environment against high threshold
  if (_environmentConfig.humhighEnable) {
    switch (humhighState) {
      case EnvState::UNKNOWN:
        humhighState = EnvState::NORMAL;
        // Fall through
      case EnvState::NORMAL: {
        if (environment.Humidity >= _environmentConfig.humhighThreshold) {
          humhighEvents++;
          humhighLatch = true;
          humhighState = EnvState::OUTSIDE_LIMIT;
        }
        break;
      }

      case EnvState::OUTSIDE_LIMIT: {
        if (environment.Humidity < _environmentConfig.humhighThreshold) {
          humhighState = EnvState::INSIDE_LIMIT;
        }
        break;
      }

      case EnvState::INSIDE_LIMIT: {
        if (environment.Humidity <= _environmentConfig.humhighThreshold - _environmentConfig.humhysteresis) {
          humhighLatch = false;
          humhighState = EnvState::NORMAL;
        }
        else if (environment.Humidity >= _environmentConfig.humhighThreshold) {
          humhighState = EnvState::OUTSIDE_LIMIT;
        }
        break;
      }
    }
  }

  // Evaluate environment against low threshold
  if (_environmentConfig.humlowEnable) {
    switch (humlowState) {
      case EnvState::UNKNOWN:
        humlowState = EnvState::NORMAL;
        // Fall through
      case EnvState::NORMAL: {
        if (environment.Humidity <= _environmentConfig.humlowThreshold) {
          humlowEvents++;
          humlowLatch = true;
          humlowState = EnvState::OUTSIDE_LIMIT;
        }
        break;
      }

      case EnvState::OUTSIDE_LIMIT: {
        if (environment.Humidity > _environmentConfig.humlowThreshold) {
          humlowState = EnvState::INSIDE_LIMIT;
        }
        break;
      }

      case EnvState::INSIDE_LIMIT: {
        if (environment.Humidity >= _environmentConfig.humlowThreshold + _environmentConfig.humhysteresis) {
          humlowLatch = false;
          humlowState = EnvState::NORMAL;
        }
        else if (environment.Humidity <= _environmentConfig.humlowThreshold) {
          humlowState = EnvState::OUTSIDE_LIMIT;
        }
        break;
      }
    }
  }

}

int environment_tick() {
  Environment environment = get_environment();
  evaluate_user_environment(environment);
  return SYSTEM_ERROR_NONE;
}