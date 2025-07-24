/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_auto_mouse

#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include <zmk/behavior.h>
#include <zmk/matrix.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/hid.h>
#include <zmk/layers.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_auto_mouse_config {
    uint8_t target_layer;
    uint16_t timeout_ms;
    bool enable_on_init;
    bool auto_enable;
};

struct behavior_auto_mouse_data {
    struct k_timer timeout_timer;
    bool layer_active;
    uint8_t original_layer;
    bool auto_mouse_enabled;
};

static void auto_mouse_timeout_handler(struct k_timer *timer) {
    struct behavior_auto_mouse_data *data = CONTAINER_OF(timer, struct behavior_auto_mouse_data, timeout_timer);
    
    if (data->layer_active) {
        LOG_DBG("Auto mouse timeout, returning to layer %d", data->original_layer);
        zmk_layers_to(data->original_layer);
        data->layer_active = false;
    }
}

static int behavior_auto_mouse_init(const struct device *dev) {
    struct behavior_auto_mouse_data *data = dev->data;
    const struct behavior_auto_mouse_config *cfg = dev->config;
    
    data->layer_active = false;
    data->original_layer = 0;
    data->auto_mouse_enabled = cfg->enable_on_init;
    
    k_timer_init(&data->timeout_timer, auto_mouse_timeout_handler, NULL);
    
    LOG_DBG("Auto mouse behavior initialized: target_layer=%d, timeout=%dms, enabled=%s",
           cfg->target_layer, cfg->timeout_ms, data->auto_mouse_enabled ? "true" : "false");
    
    return 0;
}

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                   struct zmk_behavior_binding_event binding_event) {
    const struct device *dev = zmk_behavior_get_binding_inner_dev(binding);
    struct behavior_auto_mouse_data *data = dev->data;
    const struct behavior_auto_mouse_config *cfg = dev->config;
    
    uint32_t param = binding->param1;
    
    switch (param) {
    case 0: // Toggle auto mouse enable/disable
        data->auto_mouse_enabled = !data->auto_mouse_enabled;
        LOG_DBG("Auto mouse %s", data->auto_mouse_enabled ? "enabled" : "disabled");
        
        // If disabling and layer is active, return to original layer
        if (!data->auto_mouse_enabled && data->layer_active) {
            zmk_layers_to(data->original_layer);
            data->layer_active = false;
            k_timer_stop(&data->timeout_timer);
        }
        break;
        
    case 1: // Manually activate layer
        if (data->auto_mouse_enabled && !data->layer_active) {
            data->original_layer = zmk_layers_get_current();
            zmk_layers_to(cfg->target_layer);
            data->layer_active = true;
            
            if (cfg->timeout_ms > 0) {
                k_timer_start(&data->timeout_timer, K_MSEC(cfg->timeout_ms), K_NO_WAIT);
            }
            
            LOG_DBG("Manually activated auto mouse layer %d", cfg->target_layer);
        }
        break;
        
    case 2: // Manually deactivate layer
        if (data->layer_active) {
            zmk_layers_to(data->original_layer);
            data->layer_active = false;
            k_timer_stop(&data->timeout_timer);
            LOG_DBG("Manually deactivated auto mouse layer");
        }
        break;
        
    case 3: // Reset timeout
        if (data->layer_active && cfg->timeout_ms > 0) {
            k_timer_start(&data->timeout_timer, K_MSEC(cfg->timeout_ms), K_NO_WAIT);
            LOG_DBG("Reset auto mouse timeout");
        }
        break;
        
    default:
        LOG_ERR("Invalid auto mouse parameter: %d", param);
        return -EINVAL;
    }
    
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                    struct zmk_behavior_binding_event binding_event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct zmk_behavior_driver_api behavior_auto_mouse_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

// Public API functions for use by input processors or other components
bool zmk_behavior_auto_mouse_is_enabled(const struct device *dev) {
    struct behavior_auto_mouse_data *data = dev->data;
    return data->auto_mouse_enabled;
}

bool zmk_behavior_auto_mouse_is_active(const struct device *dev) {
    struct behavior_auto_mouse_data *data = dev->data;
    return data->layer_active;
}

int zmk_behavior_auto_mouse_activate(const struct device *dev) {
    struct behavior_auto_mouse_data *data = dev->data;
    const struct behavior_auto_mouse_config *cfg = dev->config;
    
    if (!data->auto_mouse_enabled || data->layer_active) {
        return -EALREADY;
    }
    
    data->original_layer = zmk_layers_get_current();
    zmk_layers_to(cfg->target_layer);
    data->layer_active = true;
    
    if (cfg->timeout_ms > 0) {
        k_timer_start(&data->timeout_timer, K_MSEC(cfg->timeout_ms), K_NO_WAIT);
    }
    
    LOG_DBG("Activated auto mouse layer %d", cfg->target_layer);
    return 0;
}

int zmk_behavior_auto_mouse_reset_timeout(const struct device *dev) {
    struct behavior_auto_mouse_data *data = dev->data;
    const struct behavior_auto_mouse_config *cfg = dev->config;
    
    if (!data->layer_active || cfg->timeout_ms == 0) {
        return -EINVAL;
    }
    
    k_timer_start(&data->timeout_timer, K_MSEC(cfg->timeout_ms), K_NO_WAIT);
    return 0;
}

#define AUTO_MOUSE_INST(n)                                                                           \
    static struct behavior_auto_mouse_data behavior_auto_mouse_data_##n;                             \
    static struct behavior_auto_mouse_config behavior_auto_mouse_config_##n = {                      \
        .target_layer = DT_INST_PROP(n, layer),                                                     \
        .timeout_ms = DT_INST_PROP_OR(n, timeout_ms, 600),                                          \
        .enable_on_init = DT_INST_PROP_OR(n, enable_on_init, true),                                 \
        .auto_enable = DT_INST_PROP_OR(n, auto_enable, true),                                       \
    };                                                                                               \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_auto_mouse_init, NULL,                                       \
                            &behavior_auto_mouse_data_##n, &behavior_auto_mouse_config_##n,          \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                        \
                            &behavior_auto_mouse_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AUTO_MOUSE_INST)

#endif