#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(
  matter_zigbee_ui.c
  matter_zigbee_ui_led.c
)

if(CONFIG_CHIP)
  zephyr_library_sources(
    matter_zigbee_ui_button.cpp
    matter_zigbee_ui_led_matter.cpp
  )

  zephyr_library_include_directories(
    $<TARGET_PROPERTY:chip,INTERFACE_INCLUDE_DIRECTORIES>
    ${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/src
  )

  target_compile_definitions(${ZEPHYR_CURRENT_LIBRARY} PRIVATE
    $<TARGET_PROPERTY:chip,INTERFACE_COMPILE_DEFINITIONS>
  )

  add_dependencies(${ZEPHYR_CURRENT_LIBRARY} chip)

  zephyr_library_compile_options($<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17>)
  zephyr_library_compile_options(-Wno-deprecated-declarations)
endif()

zephyr_library_include_directories(
  $<TARGET_PROPERTY:zigbee,INTERFACE_INCLUDE_DIRECTORIES>
)
