/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_processor_auto_mouse_layer

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/input/input.h>
#include <drivers/input_processor.h>

#ifdef CONFIG_ZMK_LAYERS
#include <zmk/layers.h>
#include <zmk/keymap.h>
#include <zmk/events/layer_state_changed.h>
#endif

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct auto_mouse_layer_config {
    uint8_t target_layer;
    uint16_t timeout_ms;
    uint16_t movement_threshold;
    bool require_motion;
};

struct auto_mouse_layer_data {
    struct k_timer timeout_timer;
    uint16_t accumulated_movement;
    bool layer_active;
    uint8_t original_layer;
    const struct device *dev;
};

static void auto_mouse_timeout_handler(struct k_timer *timer) {
    struct auto_mouse_layer_data *data = CONTAINER_OF(timer, struct auto_mouse_layer_data, timeout_timer);
    const struct auto_mouse_layer_config *cfg = data->dev->config;

    if (data->layer_active) {
#ifdef CONFIG_ZMK_LAYERS
        LOG_DBG("Auto mouse timeout, returning to layer %d", data->original_layer);
        zmk_layers_to(data->original_layer);
#endif
        data->layer_active = false;
        data->accumulated_movement = 0;
    }
}

static void activate_auto_mouse_layer(struct auto_mouse_layer_data *data, 
                                    const struct auto_mouse_layer_config *cfg) {
    if (!data->layer_active) {
#ifdef CONFIG_ZMK_LAYERS
        data->original_layer = zmk_layers_get_current();
        zmk_layers_to(cfg->target_layer);
#endif
        data->layer_active = true;
        
        LOG_DBG("Activated auto mouse layer %d (was layer %d)", 
               cfg->target_layer, data->original_layer);
    }
    
    // Start or restart timeout timer
    if (cfg->timeout_ms > 0) {
        k_timer_start(&data->timeout_timer, K_MSEC(cfg->timeout_ms), K_NO_WAIT);
    }
}

static int auto_mouse_layer_process(const struct device *dev, struct input_event *event) {
    const struct auto_mouse_layer_config *cfg = dev->config;
    struct auto_mouse_layer_data *data = dev->data;
    
    // Only process relative movement events for auto mouse activation
    if (event->type == INPUT_EV_REL && 
        (event->code == INPUT_REL_X || event->code == INPUT_REL_Y)) {
        
        // Calculate movement magnitude
        uint16_t movement = abs(event->value);
        data->accumulated_movement += movement;
        
        LOG_DBG("Movement: %d, accumulated: %d, threshold: %d", 
               movement, data->accumulated_movement, cfg->movement_threshold);
        
        // Check if we should activate auto mouse layer
        if (!data->layer_active && data->accumulated_movement >= cfg->movement_threshold) {
            activate_auto_mouse_layer(data, cfg);
        } else if (data->layer_active && cfg->timeout_ms > 0) {
            // Reset timeout on continued movement
            k_timer_start(&data->timeout_timer, K_MSEC(cfg->timeout_ms), K_NO_WAIT);
        }
    }
    
    // Process other input events (buttons, etc.) when layer is active
    if (data->layer_active && event->type == INPUT_EV_KEY) {
        // Reset timeout on button activity
        if (cfg->timeout_ms > 0) {
            k_timer_start(&data->timeout_timer, K_MSEC(cfg->timeout_ms), K_NO_WAIT);
        }
    }
    
    // Always pass through the event
    return 0;
}

static const struct input_processor_driver_api auto_mouse_layer_api = {
    .process = auto_mouse_layer_process,
};

static int auto_mouse_layer_init(const struct device *dev) {
    struct auto_mouse_layer_data *data = dev->data;
    const struct auto_mouse_layer_config *cfg = dev->config;
    
    data->dev = dev;
    data->accumulated_movement = 0;
    data->layer_active = false;
    data->original_layer = 0;
    
    k_timer_init(&data->timeout_timer, auto_mouse_timeout_handler, NULL);
    
    LOG_DBG("Auto mouse layer processor initialized: target_layer=%d, timeout=%dms, threshold=%d",
           cfg->target_layer, cfg->timeout_ms, cfg->movement_threshold);
    
    return 0;
}

#define AUTO_MOUSE_LAYER_INST(n)                                                                      \
    static const struct auto_mouse_layer_config auto_mouse_layer_config_##n = {                       \
        .target_layer = DT_INST_PROP(n, layer),                                                      \
        .timeout_ms = DT_INST_PROP_OR(n, timeout_ms, 600),                                           \
        .movement_threshold = DT_INST_PROP_OR(n, movement_threshold, 100),                           \
        .require_motion = DT_INST_PROP_OR(n, require_motion, true),                                  \
    };                                                                                                \
                                                                                                      \
    static struct auto_mouse_layer_data auto_mouse_layer_data_##n;                                    \
                                                                                                      \
    DEVICE_DT_INST_DEFINE(n, auto_mouse_layer_init, NULL,                                            \
                          &auto_mouse_layer_data_##n, &auto_mouse_layer_config_##n,                  \
                          POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,                                    \
                          &auto_mouse_layer_api);

DT_INST_FOREACH_STATUS_OKAY(AUTO_MOUSE_LAYER_INST)