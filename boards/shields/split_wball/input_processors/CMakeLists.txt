# Copyright (c) 2025 The ZMK Contributors
# SPDX-License-Identifier: MIT

if(CONFIG_ZMK_INPUT_PROCESSOR_AUTO_MOUSE_LAYER)
  target_sources(app PRIVATE automouse_layer.c)
endif()