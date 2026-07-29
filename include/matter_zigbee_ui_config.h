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
 */

#include <dk_buttons_and_leds.h>
#include <zephyr/kernel.h>

#ifdef CONFIG_MATTER_ZIGBEE_UI

/** Factory reset hold duration.. */
#define MATTER_ZIGBEE_UI_FACTORY_RESET_PRESS_TIME                                          \
	K_SECONDS(CONFIG_MATTER_ZIGBEE_UI_FACTORY_RESET_PRESS_TIME_SECONDS)

#endif /* CONFIG_MATTER_ZIGBEE_UI */

/** @name Common button roles. */
/** @{ */
#define MATTER_ZIGBEE_UI_BUTTON_SMP_MSK             DK_BTN1_MSK
#define MATTER_ZIGBEE_UI_BUTTON_PROTOCOL_SWITCH_MSK DK_BTN3_MSK
#define MATTER_ZIGBEE_UI_BUTTON_FACTORY_RESET_MSK   DK_BTN4_MSK
#define MATTER_ZIGBEE_UI_BUTTON_IDENTIFY_MSK        DK_BTN4_MSK
/** @} */

/** @name Common LED roles. */
/** @{ */
#define MATTER_ZIGBEE_UI_LED_OTA_ACTIVITY   DK_LED2
#define MATTER_ZIGBEE_UI_LED_ZIGBEE_NETWORK DK_LED3
/** @} */

/** @} */

#endif /* MATTER_ZIGBEE_UI_CONFIG_H_ */
