/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task_matter.h"

#include "light_switch.h"

#include "app/matter_init.h"
#include "app/task_executor.h"
#include "board/board.h"
#include "clusters/identify.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/server/Server.h>
#include <setup_payload/OnboardingCodesUtil.h>

#include <matter_zigbee_coexistence.h>
#include <matter_zigbee_ui.h>
#include <matter_zigbee_ui_config.h>
#include <matter_zigbee_ui_led.h>

#include <dk_buttons_and_leds.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::DeviceLayer;

namespace
{
constexpr EndpointId kLightSwitchEndpointId = 1;
constexpr EndpointId kLightEndpointId = 1;

k_timer sLightPressTimer;
k_timer sLightDimTimer;
k_timer sIdentifyPressTimer;
k_timer sIdentifyDimTimer;

bool sLightHoldActive;
bool sIdentifyHoldActive;

class SwitchIdentifyDelegate : public Nrf::Matter::IdentifyDelegateImplNrf {
public:
	SwitchIdentifyDelegate() : IdentifyDelegateImplNrf(false, []() { matter_zigbee_ui_led_zigbee_identify_stop(); })
	{
	}

	void OnIdentifyStart(chip::app::Clusters::IdentifyCluster &cluster) override
	{
		ARG_UNUSED(cluster);
		matter_zigbee_ui_led_zigbee_identify_start();
	}

	void OnIdentifyStop(chip::app::Clusters::IdentifyCluster &cluster) override
	{
		ARG_UNUSED(cluster);
		matter_zigbee_ui_led_zigbee_identify_stop();
	}
};

SwitchIdentifyDelegate sIdentifyDelegate;
Nrf::Matter::IdentifyCluster sIdentifyCluster(kLightEndpointId, sIdentifyDelegate);

void TriggerMatterIdentify()
{
	Clusters::Identify::Attributes::IdentifyTime::Set(kLightEndpointId, 3);
}

void LightDimTimerEventHandler()
{
	LightSwitch::GetInstance().DimmerChangeBrightness(true);
}

void IdentifyDimTimerEventHandler()
{
	LightSwitch::GetInstance().DimmerChangeBrightness(false);
}

void LightPressTimeoutCallback(k_timer *timer)
{
	ARG_UNUSED(timer);

	sLightHoldActive = true;
	k_timer_start(&sLightDimTimer, K_MSEC(MATTER_ZIGBEE_UI_DIM_INTERVAL_MS),
		      K_MSEC(MATTER_ZIGBEE_UI_DIM_INTERVAL_MS));
	Nrf::PostTask([] { LightDimTimerEventHandler(); });
}

void IdentifyPressTimeoutCallback(k_timer *timer)
{
	ARG_UNUSED(timer);

	sIdentifyHoldActive = true;
	k_timer_start(&sIdentifyDimTimer, K_MSEC(MATTER_ZIGBEE_UI_DIM_INTERVAL_MS),
		      K_MSEC(MATTER_ZIGBEE_UI_DIM_INTERVAL_MS));
	Nrf::PostTask([] { IdentifyDimTimerEventHandler(); });
}

void LightDimTimeoutCallback(k_timer *timer)
{
	ARG_UNUSED(timer);
	Nrf::PostTask([] { LightDimTimerEventHandler(); });
}

void IdentifyDimTimeoutCallback(k_timer *timer)
{
	ARG_UNUSED(timer);
	Nrf::PostTask([] { IdentifyDimTimerEventHandler(); });
}

} /* namespace */

void AppTask::ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged)
{
#ifdef CONFIG_CHIP_ICD_UAT_SUPPORT
	if ((MATTER_ZIGBEE_UI_BUTTON_PROTOCOL_SWITCH_MSK & state & hasChanged)) {
		LOG_INF("ICD UserActiveMode has been triggered.");
		Server::GetInstance().GetICDManager().OnNetworkActivity();
		return;
	}
#endif

	if (hasChanged & MATTER_ZIGBEE_UI_BUTTON_LIGHT_MSK) {
		if (state & MATTER_ZIGBEE_UI_BUTTON_LIGHT_MSK) {
			sLightHoldActive = false;
			k_timer_start(&sLightPressTimer, K_MSEC(MATTER_ZIGBEE_UI_DIM_HOLD_THRESHOLD_MS), K_NO_WAIT);
		} else {
			k_timer_stop(&sLightPressTimer);
			k_timer_stop(&sLightDimTimer);
			if (!sLightHoldActive) {
				Nrf::PostTask([] {
					LightSwitch::GetInstance().InitiateActionSwitch(LightSwitch::Action::Toggle);
				});
			}
			sLightHoldActive = false;
		}
		return;
	}

	if (hasChanged & MATTER_ZIGBEE_UI_BUTTON_IDENTIFY_MSK) {
		if (state & MATTER_ZIGBEE_UI_BUTTON_IDENTIFY_MSK) {
			sIdentifyHoldActive = false;
			k_timer_start(&sIdentifyPressTimer, K_MSEC(MATTER_ZIGBEE_UI_DIM_HOLD_THRESHOLD_MS), K_NO_WAIT);
		} else {
			k_timer_stop(&sIdentifyPressTimer);
			k_timer_stop(&sIdentifyDimTimer);
			if (!sIdentifyHoldActive) {
				Nrf::PostTask([] { TriggerMatterIdentify(); });
			}
			sIdentifyHoldActive = false;
		}
	}
}

CHIP_ERROR AppTask::Init()
{
	Nrf::Matter::InitData initData{};
	initData.mPostServerInitClbk = [] {
		LightSwitch::GetInstance().Init(kLightSwitchEndpointId);
		return CHIP_NO_ERROR;
	};
	initData.mPreServerInitClbk = []() -> CHIP_ERROR {
		matter_zigbee_coexistence_pre_server_init();
		return CHIP_NO_ERROR;
	};
	ReturnErrorOnFailure(Nrf::Matter::PrepareServer(initData));

	k_timer_init(&sLightPressTimer, LightPressTimeoutCallback, nullptr);
	k_timer_init(&sLightDimTimer, LightDimTimeoutCallback, nullptr);
	k_timer_init(&sIdentifyPressTimer, IdentifyPressTimeoutCallback, nullptr);
	k_timer_init(&sIdentifyDimTimer, IdentifyDimTimeoutCallback, nullptr);

	matter_zigbee_ui_led_init();

	if (!Nrf::GetBoard().Init(ButtonEventHandler)) {
		LOG_ERR("User interface initialization failed.");
		return CHIP_ERROR_INCORRECT_STATE;
	}

	matter_zigbee_ui_button_init();
	matter_zigbee_ui_led_register_matter_events();

	ReturnErrorOnFailure(sIdentifyCluster.Init());
	ReturnErrorOnFailure(Nrf::Matter::StartServer());
	matter_zigbee_coexistence_on_server_started();

	return CHIP_NO_ERROR;
}

CHIP_ERROR AppTask::StartApp()
{
	ReturnErrorOnFailure(Init());

	while (true) {
		Nrf::DispatchNextTask();
	}

	return CHIP_NO_ERROR;
}
