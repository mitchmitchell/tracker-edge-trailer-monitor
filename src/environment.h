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

#pragma once

#include "Particle.h"
#include "tracker_config.h"
#include "config_service.h"
#include "TemperatureHumidityValidatorRK.h"

struct SensorConfig {
  float minTemperature;                 /**< Minumum temperature that can be measured with sensor. */
  float maxTemperature;                 /**< Maximum temperature that can be measured with sensor. */
  float minHumidity;                    /**< Minumum humidity that can be measured with sensor. */
  float maxHumidity;                    /**< Maximum humidity that can be measured with sensor. */
};

struct Environment {
  double Temperature;                 /**< Current temperature measured with sensor. */
  double Humidity;                    /**< Current humidity measured with sensor. */
};

// Default temperature high threshold.
constexpr double ExternalTemperatureHighDefault = 45.0; // degrees celsius

// Default temperature low threshold.
constexpr double ExternalTemperatureLowDefault = 25.0; // degrees celsius

// Default temperature hysteresis
constexpr double ExternalTemperatureHysteresisDefault = 5.0; // degrees celsius

// Default humidity high threshold.
constexpr double ExternalHumidityHighDefault = 95.0; // percent

// Default humidity low threshold.
constexpr double ExternalHumidityLowDefault = 25.0; // percent

// Default humidity hysteresis
constexpr double ExternalHumidityHysteresisDefault = 5.0; // percent

/**
 * @brief Get the current temperature and current humidity
 *
 * @return struct Environment { float Current temperature in degrees celsius, float Current humidity in percent },
 */
Environment get_environment();

/**
 * @brief Get the number of environment high threshold events since last call to this function.
 *
 * @return size_t Number of events that have elapsed.
 */
size_t environment_high_temperature_events();

/**
 * @brief Get the number of environment low threshold events since last call to this function.
 *
 * @return size_t Number of events that have elapsed.
 */
size_t environment_low_temperature_events();

/**
 * @brief Get the number of environment high threshold events since last call to this function.
 *
 * @return size_t Number of events that have elapsed.
 */
size_t environment_high_humidity_events();

/**
 * @brief Get the number of environment low threshold events since last call to this function.
 *
 * @return size_t Number of events that have elapsed.
 */
size_t environment_low_humidity_events();

/**
 * @brief Initialize the environment sampling feature.
 *
 * @param None.
 *
 * @retval SYSTEM_ERROR_NONE
 */
int environment_init();

/**
 * @brief Process the environment loop tick.
 *
 * @retval SYSTEM_ERROR_NONE
 */
int environment_tick();
