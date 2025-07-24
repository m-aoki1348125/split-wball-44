# Copyright (c) 2025 The ZMK Contributors
# SPDX-License-Identifier: MIT

# Add custom input processors and behaviors
add_subdirectory_ifdef(CONFIG_ZMK_INPUT_PROCESSOR_AUTO_MOUSE_LAYER input_processors)
add_subdirectory_ifdef(CONFIG_ZMK_BEHAVIOR_AUTO_MOUSE behaviors)