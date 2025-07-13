# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a ZMK (Zephyr Mechanical Keyboard) firmware project for a 44-key split wireless keyboard with integrated PAW3222 trackball sensor. The project was generated using Auto-Keyboard-Design-Kit (AKDK) and follows ZMK's standard architecture for custom keyboard configurations.

## Build System and Commands

### Building Firmware
```bash
# Build all targets (requires west and ZMK toolchain)
west build --pristine

# Build specific target
west build -b akdk_bt1 -- -DSHIELD=split_wball_left
west build -b akdk_bt1 -- -DSHIELD=split_wball_right
```

### GitHub Actions Build
The project uses automated building via GitHub Actions. Builds are triggered on:
- Push to any branch
- Pull requests
- Manual workflow dispatch

Build artifacts (UF2 files) are available in GitHub Actions runs.

### Development Commands
```bash
# Check build configuration
west list
west status

# Flash firmware (when connected via USB)
west flash

# Reset settings
west build -b akdk_bt1 -- -DSHIELD=settings_reset
```

## Architecture Overview

### Split Keyboard Structure
The keyboard consists of two halves with distinct roles:

**Left Half (Peripheral)**:
- 5×6 matrix with 22 active keys
- PAW3222 trackball sensor via SPI
- Transmits trackball input to right half
- GPIO mapping: rows D6,D4,D3,D1,D2 / cols D15,D14,D13,D12,D11,D5

**Right Half (Central)**:
- 5×5 matrix with 22 active keys  
- Receives and processes trackball data from left half
- Handles BLE/USB communication to host
- GPIO mapping: rows D11,D13,D14,D15,D16 / cols D12,D6,D5,D4,D3

### Key File Structure

**Board Definition** (`boards/arm/akdk_bt1/`):
- Hardware abstraction for nRF52840-based controller
- AKDK-compatible GPIO mapping (18 pins via `akdk_d` connector)
- Power management and flash partitioning

**Shield Configuration** (`boards/shields/split_wball/`):
- Keyboard-specific matrix and layout definitions
- Trackball integration via external PAW3222 driver
- Split communication setup
- Side-specific overlays and configurations

**Configuration** (`config/`):
- `keymap.keymap`: Key bindings (currently template with `&none`)
- `west.yml`: Dependency management (ZMK main + PAW3222 driver)
- `info.json`: QMK-compatible layout definition

### Trackball Integration

The PAW3222 sensor is integrated using:
- External driver module: `zmk-driver-paw3222`
- SPI interface at 2MHz on pins P0.12 (SCK), P1.9 (MOSI/MISO), P0.13 (CS)
- Input processors for movement transformation (XY to scroll wheel)
- Split input forwarding from left to right half

## Configuration Patterns

### Matrix Transform
Each half uses `zmk,matrix-transform` with:
- Column offset for right half (`col-offset = <6>`)
- Row/column mapping via `RC(row,col)` notation
- GPIO assignment through AKDK connector abstraction

### Input Processing
Trackball input flows through:
1. PAW3222 driver → raw X/Y movement
2. Input processor → transformation (e.g., XY to scroll)
3. Input listener → ZMK behavior system
4. Split communication → synchronized across halves

### Side-Specific Configuration
- `.conf` files control feature flags per side
- Central/peripheral roles assigned via `CONFIG_ZMK_SPLIT_ROLE_CENTRAL`
- Trackball enabled on both sides with different processing roles

## Development Guidelines

### Modifying Keymaps
Edit `config/keymap.keymap` to customize key bindings. The file includes references to ZMK behaviors and key codes from `<dt-bindings/zmk/keys.h>`.

### Trackball Behavior Changes
Modify input processors in shield overlay files:
- Mouse movement: Use `&zip_xy_transform` with transform flags
- Scroll wheel: Use code mapper to convert REL_X/REL_Y to REL_HWHEEL/REL_WHEEL
- Custom behaviors: Add input processor chains in device tree

### Hardware Modifications
Changes to GPIO assignments or matrix configuration require updates to:
- Board device tree (`.dts`) for pin assignments
- Shield overlays for matrix mapping
- Physical layout files for coordinate positioning

### Adding Features
New functionality typically involves:
- Device tree additions in shield overlays
- Kconfig options in shield or board directories
- Dependencies added to `west.yml` if external modules needed

## Build Artifacts

Successful builds generate UF2 firmware files for:
- `split_wball_left-akdk_bt1-zmk.uf2`
- `split_wball_right-akdk_bt1-zmk.uf2`  
- `settings_reset-akdk_bt1-zmk.uf2`

Flash these to respective halves via USB mass storage when in bootloader mode.