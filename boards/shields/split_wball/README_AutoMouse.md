# Auto Mouse Layer Implementation

This directory contains a complete implementation of auto mouse layer functionality using ZMK's input processor architecture. This provides a more modular and ZMK-standard approach compared to implementing auto mouse logic directly in pointing device drivers.

## Architecture Overview

The implementation consists of two main components:

### 1. Input Processor (`input_processors/automouse_layer.c`)
- **Purpose**: Processes input events from pointing devices to trigger layer changes
- **Features**:
  - Movement threshold detection
  - Automatic layer activation
  - Timeout-based deactivation
  - Movement accumulation tracking
- **Integration**: Works with any pointing device through ZMK's input listener system

### 2. Behavior (`behaviors/automouse.c`)
- **Purpose**: Provides manual control and API for auto mouse functionality
- **Features**:
  - Toggle auto mouse enable/disable
  - Manual layer activation/deactivation
  - Timeout reset
  - Status query API
- **Usage**: Can be bound to keys for user control

## Configuration

### Device Tree Configuration

```dts
/ {
    input_processors {
        auto_mouse_processor: auto_mouse_processor {
            compatible = "zmk,input-processor-auto-mouse-layer";
            #input-processor-cells = <0>;
            layer = <2>;                    // Target mouse layer
            timeout-ms = <800>;             // Auto-return timeout
            movement-threshold = <150>;     // Movement to trigger activation
        };
    };

    behaviors {
        auto_mouse: auto_mouse {
            compatible = "zmk,behavior-auto-mouse";
            #binding-cells = <1>;
            layer = <2>;                    // Same layer as processor
            timeout-ms = <800>;             // Same timeout as processor
            enable-on-init = <1>;           // Enable on startup
        };
    };

    trackball_listener: trackball_listener {
        compatible = "zmk,input-listener";
        device = <&trackball>;
        input-processors = <
            &auto_mouse_processor
            // ... other processors
        >;
    };
};
```

### Keymap Usage

```dts
&auto_mouse 0  // Toggle auto mouse enable/disable
&auto_mouse 1  // Manually activate mouse layer
&auto_mouse 2  // Manually deactivate mouse layer
&auto_mouse 3  // Reset timeout (extend mouse layer time)
```

## Features

### Input Processor Features
- **Movement Threshold**: Configurable threshold for activation
- **Timeout Control**: Automatic return to original layer after inactivity
- **Movement Accumulation**: Tracks total movement across multiple events
- **Event Pass-through**: All input events continue to normal processing
- **Multi-device Support**: Works with any input device

### Behavior Features  
- **Manual Control**: Key-bindable controls for all functions
- **Runtime Toggle**: Enable/disable auto mouse without reflashing
- **Status API**: Query current state for other components
- **Timeout Management**: Manual timeout reset for extended use

## Configuration Options

### Input Processor Properties
- `layer`: Target layer index (required)
- `timeout-ms`: Auto-return timeout in milliseconds (default: 600)
- `movement-threshold`: Movement required for activation (default: 100)
- `require-motion`: Whether to require motion events (default: true)

### Behavior Properties
- `layer`: Target layer index (required)
- `timeout-ms`: Auto-return timeout in milliseconds (default: 600)
- `enable-on-init`: Enable on initialization (default: true)
- `auto-enable`: Allow automatic triggering (default: true)

## Integration with Existing Systems

### PAW3222 Driver Compatibility
The original PAW3222 driver includes built-in auto mouse functionality. To use the new input processor approach instead:

1. Disable driver auto mouse by setting these properties to -1:
   ```dts
   trackball: trackball@0 {
       automouse-layer = <-1>;
       movement-threshold = <-1>;
       movement-timeout-ms = <0>;
   };
   ```

2. Add the input processor to the input listener chain

### Split Keyboard Support
Works seamlessly with split keyboards:
- Input processor runs on the central side
- Peripheral devices send raw events to central
- Layer changes affect the entire keyboard

### Input Processor Chaining
Can be combined with other input processors:
```dts
input-processors = <
    &auto_mouse_processor
    &xy_transform (INPUT_TRANSFORM_X_INVERT)
    &scroll_processor
>;
```

## Build Configuration

Add to your shield's `Kconfig.defconfig`:
```kconfig
config ZMK_INPUT_PROCESSOR_AUTO_MOUSE_LAYER
    default y

config ZMK_BEHAVIOR_AUTO_MOUSE
    default y
```

Add to your shield's `CMakeLists.txt`:
```cmake
add_subdirectory_ifdef(CONFIG_ZMK_INPUT_PROCESSOR_AUTO_MOUSE_LAYER input_processors)
add_subdirectory_ifdef(CONFIG_ZMK_BEHAVIOR_AUTO_MOUSE behaviors)
```

## Examples

See the following example files:
- `split_wball_example_with_automouse.overlay`: Device tree configuration
- `example_keymap_with_automouse.keymap`: Keymap with auto mouse bindings

## Debugging

Enable debug logging in your configuration:
```kconfig
CONFIG_ZMK_LOG_LEVEL_DBG=y
```

Look for log messages prefixed with:
- Auto mouse layer processor
- Auto mouse behavior

## Advantages over Driver-based Implementation

1. **Modularity**: Works with any pointing device
2. **Flexibility**: Can be combined with other input processors
3. **ZMK Compliance**: Uses official ZMK architecture patterns
4. **Maintainability**: Separate concerns (driver vs. behavior logic)
5. **Configurability**: Rich device tree configuration options
6. **Testability**: Components can be tested independently