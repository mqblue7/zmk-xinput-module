# ZMK XInput Module

This ZMK module adds Xbox 360 XInput gamepad support to ZMK firmware, allowing your keyboard to simultaneously function as both a HID keyboard and an XInput-compatible game controller.

## Features

- **Dual Interface**: Functions as both a keyboard and Xbox 360 controller
- **Native XInput**: Works with Windows, Linux, and any OS supporting Xbox controllers
- **ZMK Integration**: Full integration with ZMK's behavior system
- **Proper USB Initialization**: Registers XInput interface before `usb_enable()` call
- **Zephyr 4.1 Compatible**: Uses modern APIs and devicetree bindings

## Hardware Support

This module includes a sample shield for the **WeAct BlackPill STM32F411CE** with 2 buttons:
- Button 1 (PA1): Sends XInput A button
- Button 2 (PA2): Sends regular keyboard Space key

## Repository Structure

```
zmk-xinput-module/
├── zephyr/
│   └── module.yml              # Zephyr module manifest
├── include/
│   ├── zmk/
│   │   └── xinput.h            # C API header
│   └── dt-bindings/
│       └── zmk/
│           └── xinput.h        # Devicetree button constants
├── src/
│   ├── xinput_usb.c            # XInput USB driver
│   └── behavior_xinput.c       # ZMK behavior driver
├── dts/
│   └── bindings/
│       └── behaviors/
│           └── zmk,behavior-xinput.yaml
├── config/
│   ├── west.yml                # West manifest
│   └── boards/
│       └── shields/
│           └── twobutton/      # Example shield
├── CMakeLists.txt
├── Kconfig
├── build.yaml                  # Build matrix
└── .github/
    └── workflows/
        └── build.yml           # GitHub Actions workflow
```

## Usage

### Available XInput Buttons

The following button constants are available in `dt-bindings/zmk/xinput.h`:

```c
XINPUT_A           // A button
XINPUT_B           // B button
XINPUT_X           // X button
XINPUT_Y           // Y button
XINPUT_LB          // Left bumper
XINPUT_RB          // Right bumper
XINPUT_BACK        // Back button
XINPUT_START       // Start button
XINPUT_GUIDE       // Guide/Xbox button
XINPUT_LTHUMB      // Left thumbstick click
XINPUT_RTHUMB      // Right thumbstick click
XINPUT_DPAD_UP     // D-pad up
XINPUT_DPAD_DOWN   // D-pad down
XINPUT_DPAD_LEFT   // D-pad left
XINPUT_DPAD_RIGHT  // D-pad right
```

### Keymap Example

```c
#include <behaviors.dtsi>
#include <dt-bindings/zmk/keys.h>
#include "dt-bindings/zmk/xinput.h"

/ {
    keymap {
        compatible = "zmk,keymap";
        
        default_layer {
            bindings = <
                &xin XINPUT_A      // XInput A button
                &xin XINPUT_B      // XInput B button
                &kp SPACE          // Regular keyboard Space
                &kp ENTER          // Regular keyboard Enter
            >;
        };
    };
};
```

### Shield Overlay Example

```dts
#include <dt-bindings/zmk/matrix_transform.h>

/ {
    chosen {
        zmk,kscan = &kscan0;
    };

    kscan0: kscan {
        compatible = "zmk,kscan-gpio-direct";
        wakeup-source;
        input-gpios
            = <&gpioa 1 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>
            , <&gpioa 2 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>
            ;
    };

    xin: behavior_xinput {
        compatible = "zmk,behavior-xinput";
        #binding-cells = <1>;
    };
};
```

## Building

### GitHub Actions (Recommended)

1. Fork this repository
2. Push changes to your fork
3. GitHub Actions will automatically build the firmware
4. Download the firmware from the Actions artifacts

### Local Build

```bash
# Clone with west
west init -l config
west update
west zephyr-export

# Build for BlackPill F411
west build -s zmk/app -b weact_mini_f411ce@blackpill -- -DSHIELD=twobutton
```

## Technical Details

### USB Initialization Sequence

The module carefully handles USB initialization to ensure both the HID keyboard and XInput interfaces are available:

1. **POST_KERNEL, Priority 0**: XInput USB registers via `usb_set_config()`
2. **APPLICATION Level**: ZMK calls `usb_enable()` once, seeing both interfaces

This ensures Windows/Linux properly enumerate both the keyboard and gamepad.

### USB Descriptors

The module implements a proper Xbox 360 controller:
- **VID**: 0x045E (Microsoft)
- **PID**: 0x028E (Xbox 360 Controller)
- **Vendor Class**: 0xFF/0x5D/0x01
- **Endpoints**: Interrupt IN/OUT at 0x83/0x03

### Compatibility

- **Zephyr**: 4.1+
- **ZMK**: Latest main branch
- **Tested Boards**: WeAct BlackPill STM32F411CE
- **OS Support**: Windows, Linux, macOS (any OS with XInput support)

## Extending

To add XInput support to your own keyboard:

1. Add this module to your `config/west.yml`:
```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: xinput-module
      url-base: https://github.com/yourusername
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    - name: zmk-xinput-module
      remote: xinput-module
      revision: main
  self:
    path: config
```

2. Enable in your shield's `Kconfig.defconfig`:
```kconfig
config ZMK_XINPUT
    default y
```

3. Add the behavior to your overlay:
```dts
xin: behavior_xinput {
    compatible = "zmk,behavior-xinput";
    #binding-cells = <1>;
};
```

4. Use `&xin XINPUT_*` in your keymap

## License

MIT License - see individual files for copyright notices.

## Contributing

Contributions welcome! Please ensure:
- Code follows ZMK style guidelines
- Compatibility with Zephyr 4.1+
- No deprecated APIs
- All includes use `zephyr/` prefix

## Support

For issues or questions:
- Open an issue on GitHub
- Check ZMK documentation: https://zmk.dev
- Join ZMK Discord: https://zmk.dev/community/discord/invite
