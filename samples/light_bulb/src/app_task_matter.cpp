/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task_matter.h"

#include "app/matter_init.h"
#include "app/task_executor.h"

#if defined(CONFIG_PWM)
#include "pwm/pwm_device.h"
#endif

#include "clusters/identify.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/persistence/AttributePersistenceProviderInstance.h>
#include <app/persistence/DefaultAttributePersistenceProvider.h>
#include <app/persistence/DeferredAttributePersistenceProvider.h>
#include <app/server/Server.h>
#include <setup_payload/OnboardingCodesUtil.h>

#if defined(CONFIG_MATTER_ZIGBEE_COEXISTENCE)
#include <matter_zigbee_coexistence.h>
#endif

#include <matter_zigbee_ui.h>
#include <matter_zigbee_ui_config.h>
#include <matter_zigbee_ui_led.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::DeviceLayer;

namespace
{
constexpr EndpointId kLightEndpointId = 1;
constexpr uint8_t kDefaultMinLevel = 0;
constexpr uint8_t kDefaultMaxLevel = 254;

k_timer sLightPressTimer;
k_timer sLightDimTimer;
k_timer sIdentifyPressTimer;
k_timer sIdentifyDimTimer;

bool sLightHoldActive;
bool sIdentifyHoldActive;

class BulbIdentifyDelegate : public Nrf::Matter::IdentifyDelegateImplNrf {
public:
	BulbIdentifyDelegate()
		: IdentifyDelegateImplNrf(false, []() {
			  Nrf::PostTask([] {
#if defined(CONFIG_PWM)
				  AppTask::Instance().GetPWMDevice().ApplyLevel();
#else
				  Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3).Set(false);
#endif
			  });
		  })
	{
	}

	void OnIdentifyStart(chip::app::Clusters::IdentifyCluster &cluster) override
	{
		ARG_UNUSED(cluster);
		Nrf::PostTask([] {
#if defined(CONFIG_PWM)
			static bool blink_on;
			blink_on = !blink_on;
			if (blink_on) {
				AppTask::Instance().GetPWMDevice().SetLevel(254, 0);
			} else {
				AppTask::Instance().GetPWMDevice().SetLevel(0, 0);
			}
#else
			Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3).Set(
				!Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3).GetState());
#endif
		});
	}

	void OnIdentifyStop(chip::app::Clusters::IdentifyCluster &cluster) override
	{
		IdentifyDelegateImplNrf::OnIdentifyStop(cluster);
	}
};

BulbIdentifyDelegate sIdentifyDelegate;
Nrf::Matter::IdentifyCluster sIdentifyCluster(kLightEndpointId, sIdentifyDelegate);

#if defined(CONFIG_PWM)
const struct pwm_dt_spec sLightPwmDevice = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led1));
#endif

DeferredAttribute gCurrentLevelPersister(ConcreteAttributePath(kLightEndpointId, Clusters::LevelControl::Id,
							       Clusters::LevelControl::Attributes::CurrentLevel::Id));
DefaultAttributePersistenceProvider gSimpleAttributePersistence;
DeferredAttributePersistenceProvider gDeferredAttributePersister(gSimpleAttributePersistence,
								 Span<DeferredAttribute>(&gCurrentLevelPersister, 1),
								 System::Clock::Milliseconds32(5000));

void TriggerMatterIdentify()
{
	Clusters::Identify::Attributes::IdentifyTime::Set(kLightEndpointId, 3);
}

void LightDimTimerEventHandler()
{
#if defined(CONFIG_PWM)
	auto &pwm = AppTask::Instance().GetPWMDevice();
	uint8_t level = pwm.GetLevel();

	level = (level + 10 > 254) ? 254 : level + 10;
	pwm.InitiateAction(Nrf::PWMDevice::LEVEL_ACTION, static_cast<int32_t>(LightingActor::Button), &level);
#endif
}

void IdentifyDimTimerEventHandler()
{
#if defined(CONFIG_PWM)
	auto &pwm = AppTask::Instance().GetPWMDevice();
	uint8_t level = pwm.GetLevel();

	level = (level < 10) ? 0 : level - 10;
	pwm.InitiateAction(Nrf::PWMDevice::LEVEL_ACTION, static_cast<int32_t>(LightingActor::Button), &level);
#endif
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

void AppTask::LightingActionEventHandler(const LightingEvent &event)
{
#if defined(CONFIG_PWM)
	Nrf::PWMDevice::Action_t action = Nrf::PWMDevice::INVALID_ACTION;
	int32_t actor = 0;
	if (event.Actor == LightingActor::Button) {
		action = Instance().mPWMDevice.IsTurnedOn() ? Nrf::PWMDevice::OFF_ACTION : Nrf::PWMDevice::ON_ACTION;
		actor = static_cast<int32_t>(event.Actor);
	}

	if (action == Nrf::PWMDevice::INVALID_ACTION || !Instance().mPWMDevice.InitiateAction(action, actor, NULL)) {
		LOG_INF("An action could not be initiated.");
	}
#else
	Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3).Set(!Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3).GetState());
#endif
}

void AppTask::ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged)
{
	if (hasChanged & MATTER_ZIGBEE_UI_BUTTON_LIGHT_MSK) {
		if (state & MATTER_ZIGBEE_UI_BUTTON_LIGHT_MSK) {
			sLightHoldActive = false;
			k_timer_start(&sLightPressTimer, K_MSEC(MATTER_ZIGBEE_UI_DIM_HOLD_THRESHOLD_MS), K_NO_WAIT);
		} else {
			k_timer_stop(&sLightPressTimer);
			k_timer_stop(&sLightDimTimer);
			if (!sLightHoldActive) {
				Nrf::PostTask([] {
					LightingEvent event;
					event.Actor = LightingActor::Button;
					LightingActionEventHandler(event);
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

#if defined(CONFIG_PWM)
void AppTask::ActionInitiated(Nrf::PWMDevice::Action_t action, int32_t actor)
{
	ARG_UNUSED(action);
	ARG_UNUSED(actor);
}

void AppTask::ActionCompleted(Nrf::PWMDevice::Action_t action, int32_t actor)
{
	if (actor == static_cast<int32_t>(LightingActor::Button)) {
		Instance().UpdateClusterState();
	}
	ARG_UNUSED(action);
}
#endif

void AppTask::UpdateClusterState()
{
	SystemLayer().ScheduleLambda([this] {
#if defined(CONFIG_PWM)
		Protocols::InteractionModel::Status status =
			Clusters::OnOff::Attributes::OnOff::Set(kLightEndpointId, mPWMDevice.IsTurnedOn());
		if (status == Protocols::InteractionModel::Status::Success) {
			status = Clusters::LevelControl::Attributes::CurrentLevel::Set(kLightEndpointId,
										       mPWMDevice.GetLevel());
		}
#else
		Protocols::InteractionModel::Status status = Clusters::OnOff::Attributes::OnOff::Set(
			kLightEndpointId, Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3).GetState());
#endif
		if (status != Protocols::InteractionModel::Status::Success) {
			LOG_ERR("Updating clusters failed: %x", to_underlying(status));
		}
	});
}

void AppTask::InitPWMDDevice()
{
#if defined(CONFIG_PWM)
	uint8_t minLightLevel = kDefaultMinLevel;
	Clusters::LevelControl::Attributes::MinLevel::Get(kLightEndpointId, &minLightLevel);

	uint8_t maxLightLevel = kDefaultMaxLevel;
	Clusters::LevelControl::Attributes::MaxLevel::Get(kLightEndpointId, &maxLightLevel);

	Clusters::LevelControl::Attributes::CurrentLevel::TypeInfo::Type currentLevel;
	Clusters::LevelControl::Attributes::CurrentLevel::Get(kLightEndpointId, currentLevel);

	int ret =
		mPWMDevice.Init(&sLightPwmDevice, minLightLevel, maxLightLevel, currentLevel.ValueOr(kDefaultMaxLevel));
	if (ret != 0) {
		LOG_ERR("Failed to initialize PWM device.");
	}

	mPWMDevice.SetCallbacks(ActionInitiated, ActionCompleted);
#endif
}

CHIP_ERROR AppTask::Init()
{
	Nrf::Matter::InitData initData{};
	initData.mPostServerInitClbk = []() {
		app::SetAttributePersistenceProvider(&gDeferredAttributePersister);
		gSimpleAttributePersistence.Init(Nrf::Matter::GetPersistentStorageDelegate());
		return CHIP_NO_ERROR;
	};
#if defined(CONFIG_MATTER_ZIGBEE_COEXISTENCE)
	initData.mPreServerInitClbk = []() -> CHIP_ERROR {
		matter_zigbee_coexistence_pre_server_init();
		return CHIP_NO_ERROR;
	};
#endif
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

#if defined(CONFIG_MATTER_ZIGBEE_COEXISTENCE)
	matter_zigbee_coexistence_on_server_started();
#endif

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
