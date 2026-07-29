/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MATTER_ZIGBEE_UI_H_
#define MATTER_ZIGBEE_UI_H_

/** @file
 * @defgroup matter_zigbee_ui Combined sample UI module
 * @{
 *
 * @brief Central initialization for shared button roles in combined samples.
 *
 * Call @ref matter_zigbee_ui_button_init from the Matter application after
 * @c Nrf::GetBoard().Init(). Call @ref matter_zigbee_ui_on_matter_board_ready
 * from the coexistence @c post_matter_board_init hook to chain sample-specific
 * Zigbee button handlers.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** DK buttons callback type. */
typedef void (*matter_zigbee_ui_button_handler_t)(uint32_t button_state, uint32_t has_changed);

/** @brief Register Button 0 SMP and unified factory reset handlers. */
void matter_zigbee_ui_button_init(void);

/** @brief Chain a sample button handler on the DK buttons library. */
void matter_zigbee_ui_register_button_handler(matter_zigbee_ui_button_handler_t handler);

/** @brief Chain sample Zigbee button handlers after Matter @c Board::Init(). */
void matter_zigbee_ui_on_matter_board_ready(matter_zigbee_ui_button_handler_t sample_handler);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* MATTER_ZIGBEE_UI_H_ */
