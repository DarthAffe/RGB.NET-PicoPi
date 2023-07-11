# RGB.NET-PicoPi
Firmware for the Raspberry Pi Pico (RP2040) to be used with [RGB.NET](https://github.com/DarthAffe/RGB.NET).


Building the firmware requires the [pico sdk](https://github.com/raspberrypi/pico-sdk).

For precompiled binaries check https://github.com/DarthAffe/RGB.NET-PicoPi/releases

## How to use
1. Load the Pico in boot-mode (pressing the small button on startup) and copy the firmware file (RGB.NET.uf2) onto it.
2. Restart the Pico and open the Config-tool. Configure the amount of leds (set channels to 0 if unused) and if needed the used pins.
3. Restart the Pico

The HID-endpoint is usable with default drivers on windows, if you want to use the Bulk-endpoint (better performance for higher amounts of leds) you need to install the libusb driver for it using Zadig (https://zadig.akeo.ie/).
libusb-win32 works best for me, but the others should work too.

If you're using linux you'll likely need to run applications using this device as root. This can be prevented by changing the read/write permissions of the PicoPi-Device by adding a new udev-rule (for example `85-rgbnet.rules`) in your udev rules directory (most likely something like `/etc/udev/rules.d` containing this single line:   
`SUBSYSTEMS=="usb", ATTRS{idVendor}=="1209", ATTRS{idProduct}=="2812", MODE="666"`
