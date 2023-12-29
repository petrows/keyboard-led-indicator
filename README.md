# keyboard-led-indicator
Simple firmware for STM32 / Zephyr RTOS to indicate keyboard LEDs.

Implements simple USB HID Keyboard, which used to display CapsLock,
ScrollLock and NumLock as external LEDs. Used as external indicator
for desktop lock status and alernative layout activity.

See blog posts (russian language):

* [Original idea of LED indicators for desktop lock status and layout](https://petro.ws/keyboard-indicator)
* [Extend with this DIY device](https://petro.ws/diy-usb-keyboard-led)
* [Using this device to indicate layout and lock as Railroad model](https://mysku.club/blog/diy/98606.html)

## Hardware

Based on cheap STM32 board, known as [Bluepill with Cortex M3](https://www.az-delivery.de/en/products/stm32f103c8t6).

I used J-Link Segger clone from Aliexpress to flash and debug.

### Device endpoints

Device has 2 HID devices:

* 0: Keyboard, reads LED
* 1: Generic HID, first endpoint input (0x01) seel "Custom HID endpoint protocol" below

### UDEV rules

See file ``00-udev-device.rules`` provided in repo, copy to ``/etc/udev/rules.d/`` folder.
It will create 2 device symlinks (to interface 1 - generic HID):

* Generic ``/dev/hidraw-led-indicator`` for each connected
* Explicit ``/dev/hidraw-led-indicator-<serial-number>`` for each connected

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
* Led A -> A3, switched by endpoint input 0x01
* Led B -> A4, switched by endpoint input 0x01
* Led C -> A5, switched by endpoint input 0x01

## Custom HID endpoint protocol

Custom endpoint 0x01 reads 2 bytes:

* Byte 0: Command ID
* Byte 1: Command data

### Command to switch LED channel

* Command ID: ``0x01``
* Command data:
    * Bit 0: LED A state
    * Bit 1: LED B state
    * Bit 2: LED C state

Example:

```bash
# Switch LED A ON, others OFF
echo -ne '\x01\x01' > /dev/hidraw-led-indicator
# Switch all leds ON
echo -ne '\x01\x07' > /dev/hidraw-led-indicator
# Switch all leds OFF
echo -ne '\x01\x00' > /dev/hidraw-led-indicator
```
