# keyboard-led-indicator
Simple firmware for STM32 / Zephyr RTOS to indicate keyboard LEDs.

Implements simple USB HID Keyboard, which used to display CapsLock,
ScrollLock and NumLock as external LEDs. Used as external indicator
for desktop lock status and alernative layout activity.

See blog posts (russian language):

* [Original idea of LED indicators for desktop lock status and layout](https://petro.ws/keyboard-indicator)
* [Extend with this DIY device](https://petro.ws/diy-usb-keyboard-led)

## Hardware

Based on cheap STM32 board, known as [Bluepill with Cortex M3](https://www.az-delivery.de/en/products/stm32f103c8t6).

I used J-Link Segger clone from Aliexpress to flash and debug.

### Device endpoints

Device has 2 HID devices:

* 0: Keyboard, reads LED
* 1: Generic HID, first endpoint input (0xF2) reads byte and toggles LED A and B, A if last bit set, else B

## Buildsystem

Buildsystem is Platform.io. To build, just open this project in VSCode
with PlatformIO plugin, or call build manually:

```bash
platformio run --environment bluepill_f103c8
```

## Framework and software

Based on Zephyr RTOS framework for easy USB HID implementation.

LEDs are configured via DeviceTree, see file ``zephyr/board.overlay`` for
pinout configuration.

In my config:

* CapsLock -> A0, indicates russuan layout active
* NumLock -> A1, indicates NumLock
* ScrollLock -> A2, indicates desktop lock status
* Led A -> A4, switched by endpoint input 0xF2
* Led B -> A5, switched by endpoint input 0xF2
