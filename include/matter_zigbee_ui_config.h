/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MATTER_ZIGBEE_UI_CONFIG_H_
#define MATTER_ZIGBEE_UI_CONFIG_H_

/** @file
 * @defgroup matter_zigbee_ui_config Combined sample UI roles
 * @{
 *
 * @brief Shared button and LED roles for the combined Matter + Zigbee samples.
 *
 * Physical mapping (README numbering):
 * - Button 0 / LED 0 = DK_BTN1 / DK_LED1
 * - Button 1 / LED 1 = DK_BTN2 / DK_LED2
 * - Button 2 / LED 2 = DK_BTN3 / DK_LED3
 * - Button 3 / LED 3 = DK_BTN4 / DK_LED4
 */

#include <dk_buttons_and_leds.h>
#include <zephyr/kernel.h>

#ifdef CONFIG_MATTER_ZIGBEE_UI

/** Factory reset hold duration. */
#define MATTER_ZIGBEE_UI_FACTORY_RESET_PRESS_TIME                                          \
	K_SECONDS(CONFIG_MATTER_ZIGBEE_UI_FACTORY_RESET_PRESS_TIME_SECONDS)

#endif /* CONFIG_MATTER_ZIGBEE_UI */

/** @name Button roles. */
/** @{ */
#define MATTER_ZIGBEE_UI_BUTTON_SMP_MSK             DK_BTN1_MSK
#define MATTER_ZIGBEE_UI_BUTTON_FACTORY_RESET_MSK   DK_BTN1_MSK
#define MATTER_ZIGBEE_UI_BUTTON_PROTOCOL_SWITCH_MSK DK_BTN2_MSK
#define MATTER_ZIGBEE_UI_BUTTON_LIGHT_MSK           DK_BTN3_MSK
#define MATTER_ZIGBEE_UI_BUTTON_IDENTIFY_MSK        DK_BTN4_MSK
/** @} */

/** @name LED roles. */
/** @{ */
#define MATTER_ZIGBEE_UI_LED_MATTER DK_LED1
#define MATTER_ZIGBEE_UI_LED_ZIGBEE DK_LED2
#define MATTER_ZIGBEE_UI_LED_LIGHT  DK_LED3
/** @} */

/** @name Light control timing. */
/** @{ */
#define MATTER_ZIGBEE_UI_DIM_HOLD_THRESHOLD_MS 500
#define MATTER_ZIGBEE_UI_DIM_INTERVAL_MS      300
#define MATTER_ZIGBEE_UI_DIM_STEP             15
/** @} */

/** @} */

#endif /* MATTER_ZIGBEE_UI_CONFIG_H_ */
