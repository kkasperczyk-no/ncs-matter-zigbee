/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <matter_zigbee_ui.h>

#include <dk_buttons_and_leds.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(matter_zigbee_ui, CONFIG_MATTER_ZIGBEE_UI_LOG_LEVEL);

void matter_zigbee_ui_register_button_handler(matter_zigbee_ui_button_handler_t handler)
{
	static struct button_handler dk_handler;
	static bool sample_handler_registered;

	if (handler == NULL || sample_handler_registered) {
		return;
	}

	dk_handler.cb = handler;
	dk_button_handler_add(&dk_handler);
	sample_handler_registered = true;
}

void matter_zigbee_ui_on_matter_board_ready(matter_zigbee_ui_button_handler_t sample_handler)
{
	if (sample_handler != NULL) {
		matter_zigbee_ui_register_button_handler(sample_handler);
	}
}
