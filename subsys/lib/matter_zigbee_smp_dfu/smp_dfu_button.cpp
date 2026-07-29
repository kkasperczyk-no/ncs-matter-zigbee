/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <matter_zigbee_smp_dfu.h>
#include <matter_zigbee_ui_config.h>

#include <app/server/Server.h>
#include <app/task_executor.h>
#include <board/board.h>

#include <dfu/smp/dfu_over_smp.h>
#include <dk_buttons_and_leds.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(matter_zigbee_smp_dfu, CONFIG_MATTER_ZIGBEE_SMP_DFU_LOG_LEVEL);

namespace
{

void StartBleAdvertisementOnButtonPress()
{
	if (chip::Server::GetInstance().GetFabricTable().FabricCount() > 0) {
		Nrf::GetDFUOverSMP().StartServer();
		return;
	}

	Nrf::Board::StartBLEAdvertisement();
}

void SmpDfuButtonHandler(uint32_t button_state, uint32_t has_changed)
{
	if (!(has_changed & MATTER_ZIGBEE_UI_BUTTON_SMP_MSK) ||
	    !(button_state & MATTER_ZIGBEE_UI_BUTTON_SMP_MSK)) {
		return;
	}

	Nrf::PostTask([] { StartBleAdvertisementOnButtonPress(); });
}

void RegisterSmpDfuButtonHandlerOnce()
{
	static bool registered;

	if (registered) {
		return;
	}

	static struct button_handler handler = {
		.cb = SmpDfuButtonHandler,
	};

	dk_button_handler_add(&handler);
	registered = true;
}

} /* namespace */

extern "C" void matter_zigbee_smp_dfu_init(void)
{
	RegisterSmpDfuButtonHandlerOnce();
}
