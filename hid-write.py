#!/usr/bin/python

import usb.core
import usb.util
from pprint import pprint

dev = usb.core.find(idVendor=0xF109, idProduct=0x0001)

pprint(list(dev))

if dev.is_kernel_driver_active(0):
    print( 'detaching kernel driver')
    dev.detach_kernel_driver(0)

for d in dev:
    pprint(d)

dev.set_configuration()

# # get an endpoint instance
cfg = dev.get_active_configuration()
intf = cfg[(1,0)]

# # endpoint_in = dev[1][(0,0)][0]
endpoint_out = dev[0][(1,0)][1]


# # Send a command to the Teensy
endpoint_out.write( b'\x01' )

