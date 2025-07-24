# Copyright (c) 2025 The ZMK Contributors
# SPDX-License-Identifier: MIT

if(CONFIG_ZMK_BEHAVIOR_AUTO_MOUSE)
  target_sources(app PRIVATE automouse.c)
endif()