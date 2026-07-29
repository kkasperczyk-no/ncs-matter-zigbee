/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MATTER_ZIGBEE_UI_LED_H_
#define MATTER_ZIGBEE_UI_LED_H_

/** @file
 * @defgroup matter_zigbee_ui_led Combined sample LED roles
 * @{
 *
 * @brief Protocol status and identify LED helpers for combined samples.
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <zboss_api.h>

/** @brief Initialize DK LEDs used by the UI module. */
void matter_zigbee_ui_led_init(void);

/** @brief Update Zigbee network status LED from a ZBOSS signal buffer. */
void matter_zigbee_ui_led_zigbee_signal(zb_bufid_t bufid);

/** @brief Set bulb-found indicator on the switch (LED 2). */
void matter_zigbee_ui_led_set_bulb_found(bool found);

/** @brief Register Matter connectivity updates for LED 0. */
void matter_zigbee_ui_led_register_matter_events(void);

/** @brief Blink the sample identify LED during local Zigbee identify. */
void matter_zigbee_ui_led_zigbee_identify_start(void);

/** @brief Restore LEDs after local Zigbee identify ends. */
void matter_zigbee_ui_led_zigbee_identify_stop(void);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* MATTER_ZIGBEE_UI_LED_H_ */
