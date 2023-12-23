#!/usr/bin/python3

import time

device = open('/dev/hidraw-led-indicator', 'wb', buffering=0)

while True:
    # Led A on, led B off
    device.write(b'\x00\x01')
    time.sleep(1)
    # Led A off, led B on
    device.write(b'\x00\x02')
    time.sleep(1)
