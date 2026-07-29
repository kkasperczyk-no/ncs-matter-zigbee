/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <matter_zigbee_ui_led.h>
#include <matter_zigbee_ui_config.h>

#include <dk_buttons_and_leds.h>
#include <zigbee/zigbee_app_utils.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(matter_zigbee_ui, CONFIG_MATTER_ZIGBEE_UI_LOG_LEVEL);

static struct k_timer identify_timer;
static bool identify_active;
static bool bulb_found;

static void identify_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	static int blink;

	dk_set_led(MATTER_ZIGBEE_UI_LED_ZIGBEE, (++blink) % 2);
}

void matter_zigbee_ui_led_init(void)
{
	int err = dk_leds_init();

	if (err) {
		LOG_ERR("Cannot init LEDs (err: %d)", err);
	}

	k_timer_init(&identify_timer, identify_timer_handler, NULL);
}

void matter_zigbee_ui_led_zigbee_signal(zb_bufid_t bufid)
{
	if (identify_active) {
		return;
	}

	zigbee_led_status_update(bufid, MATTER_ZIGBEE_UI_LED_ZIGBEE);
}

void matter_zigbee_ui_led_set_bulb_found(bool found)
{
	bulb_found = found;

	if (identify_active) {
		return;
	}

	if (found) {
		dk_set_led_on(MATTER_ZIGBEE_UI_LED_LIGHT);
	} else {
		dk_set_led_off(MATTER_ZIGBEE_UI_LED_LIGHT);
	}
}

void matter_zigbee_ui_led_zigbee_identify_start(void)
{
	identify_active = true;
	k_timer_start(&identify_timer, K_MSEC(100), K_MSEC(100));
}

void matter_zigbee_ui_led_zigbee_identify_stop(void)
{
	k_timer_stop(&identify_timer);
	identify_active = false;

	if (bulb_found) {
		dk_set_led_on(MATTER_ZIGBEE_UI_LED_LIGHT);
	} else {
		dk_set_led_off(MATTER_ZIGBEE_UI_LED_LIGHT);
	}

	if (ZB_JOINED()) {
		dk_set_led_on(MATTER_ZIGBEE_UI_LED_ZIGBEE);
	} else {
		dk_set_led_off(MATTER_ZIGBEE_UI_LED_ZIGBEE);
	}
}
