/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <matter_zigbee_ui.h>
#include <matter_zigbee_ui_config.h>

#include <app/server/Server.h>
#include <app/task_executor.h>
#include <board/board.h>

#include <dfu/smp/dfu_over_smp.h>
#include <dk_buttons_and_leds.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(matter_zigbee_ui, CONFIG_MATTER_ZIGBEE_UI_LOG_LEVEL);

namespace
{

constexpr uint32_t kButton0Mask = MATTER_ZIGBEE_UI_BUTTON_SMP_MSK;
constexpr k_timeout_t kFactoryResetProbeInterval = K_MSEC(100);
constexpr int64_t kFactoryResetPressMs =
	CONFIG_MATTER_ZIGBEE_UI_FACTORY_RESET_PRESS_TIME_SECONDS * 1000LL;

K_TIMER_DEFINE(s_press_timer, nullptr, nullptr);

atomic_t s_button0_pressed;
atomic_t s_factory_reset_triggered;
int64_t s_press_start_ms;
bool s_handler_registered;

void StartBleAdvertisementOnButtonPress()
{
	if (chip::Server::GetInstance().GetFabricTable().FabricCount() > 0) {
		Nrf::GetDFUOverSMP().StartServer();
		return;
	}

	Nrf::Board::StartBLEAdvertisement();
}

void TriggerUnifiedFactoryReset()
{
	LOG_INF("Unified factory reset triggered (Button 0 long press)");
	chip::Server::GetInstance().ScheduleFactoryReset();
}

void PressTimerHandler(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	uint32_t button_state = 0;
	uint32_t has_changed = 0;

	if (atomic_get(&s_button0_pressed) == 0) {
		return;
	}

	dk_read_buttons(&button_state, &has_changed);

	if (!(button_state & kButton0Mask)) {
		k_timer_stop(&s_press_timer);
		atomic_set(&s_button0_pressed, 0);
		return;
	}

	if ((k_uptime_get() - s_press_start_ms) >= kFactoryResetPressMs) {
		atomic_set(&s_factory_reset_triggered, 1);
		k_timer_stop(&s_press_timer);
		Nrf::PostTask([] { TriggerUnifiedFactoryReset(); });
	}
}

void Button0Handler(uint32_t button_state, uint32_t has_changed)
{
	if (!(has_changed & kButton0Mask)) {
		return;
	}

	if (button_state & kButton0Mask) {
		atomic_set(&s_factory_reset_triggered, 0);
		atomic_set(&s_button0_pressed, 1);
		s_press_start_ms = k_uptime_get();
		k_timer_start(&s_press_timer, kFactoryResetProbeInterval, kFactoryResetProbeInterval);
		return;
	}

	k_timer_stop(&s_press_timer);
	atomic_set(&s_button0_pressed, 0);

	if (atomic_get(&s_factory_reset_triggered) != 0) {
		LOG_DBG("Button 0 released after factory reset was triggered");
		return;
	}

	LOG_DBG("Button 0 short press - starting SMP BLE advertising");
	Nrf::PostTask([] { StartBleAdvertisementOnButtonPress(); });
}

void RegisterButton0HandlerOnce()
{
	if (s_handler_registered) {
		return;
	}

	k_timer_init(&s_press_timer, PressTimerHandler, nullptr);

	static struct button_handler handler = {
		.cb = Button0Handler,
	};

	dk_button_handler_add(&handler);
	s_handler_registered = true;
}

} /* namespace */

extern "C" void matter_zigbee_ui_button_init(void)
{
	RegisterButton0HandlerOnce();
}
