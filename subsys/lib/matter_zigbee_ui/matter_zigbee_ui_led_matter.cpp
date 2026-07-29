/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <matter_zigbee_ui_led.h>
#include <matter_zigbee_ui_config.h>

#include <app/matter_event_handler.h>
#include <app/server/Server.h>
#include <lib/support/logging/CHIPLogging.h>
#include <platform/CHIPDeviceLayer.h>

#include <dk_buttons_and_leds.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(matter_zigbee_ui, CONFIG_MATTER_ZIGBEE_UI_LOG_LEVEL);

namespace
{

enum class MatterLedState : uint8_t { Off, Blink, Solid };

MatterLedState s_matter_led_state = MatterLedState::Off;
bool s_matter_led_phase;

void ApplyMatterLed(void)
{
	switch (s_matter_led_state) {
	case MatterLedState::Off:
		dk_set_led_off(MATTER_ZIGBEE_UI_LED_MATTER);
		break;
	case MatterLedState::Solid:
		dk_set_led_on(MATTER_ZIGBEE_UI_LED_MATTER);
		break;
	case MatterLedState::Blink:
		dk_set_led(MATTER_ZIGBEE_UI_LED_MATTER, s_matter_led_phase);
		break;
	}
}

void SetMatterLedState(MatterLedState state)
{
	if (s_matter_led_state == state) {
		return;
	}

	s_matter_led_state = state;
	s_matter_led_phase = false;
	ApplyMatterLed();
}

void MatterLedEventHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg)
{
	ARG_UNUSED(arg);

	using namespace chip::DeviceLayer;

	static bool network_provisioned;
	static bool ble_connected;

	switch (event->Type) {
	case DeviceEventType::kCHIPoBLEAdvertisingChange:
		ble_connected = ConnectivityMgr().NumBLEConnections() != 0;
		break;
	case DeviceEventType::kThreadStateChange:
	case DeviceEventType::kWiFiConnectivityChange:
		network_provisioned = ConnectivityMgrImpl().IsIPv6NetworkProvisioned() &&
				      ConnectivityMgrImpl().IsIPv6NetworkEnabled();
		break;
	default:
		break;
	}

	if (network_provisioned) {
		SetMatterLedState(MatterLedState::Solid);
	} else if (ble_connected || ConnectivityMgr().IsBLEAdvertising()) {
		if (s_matter_led_state != MatterLedState::Blink) {
			SetMatterLedState(MatterLedState::Blink);
		}
		s_matter_led_phase = !s_matter_led_phase;
		ApplyMatterLed();
	} else {
		SetMatterLedState(MatterLedState::Off);
	}
}

} /* namespace */

extern "C" void matter_zigbee_ui_led_register_matter_events(void)
{
	CHIP_ERROR err = Nrf::Matter::RegisterEventHandler(MatterLedEventHandler, 0);

	if (err != CHIP_NO_ERROR) {
		LOG_ERR("Failed to register Matter LED handler: %" CHIP_ERROR_FORMAT, err.Format());
	}
}
