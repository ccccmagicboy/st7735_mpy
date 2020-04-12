import machine
from machine import Pin
import st7735
import time
import random
import vga1_8x8 as font

spi = machine.SPI(2, baudrate=10000000, polarity=1, phase=1, sck=Pin(32), mosi=Pin(5), miso=Pin(36))
display = st7735.ST7735(spi, 80, 160, reset=machine.Pin(33, machine.Pin.OUT), dc=machine.Pin(4, machine.Pin.OUT))
display.init()

while True:
    display.fill(
        st7735.color565(
            random.getrandbits(8),
            random.getrandbits(8),
            random.getrandbits(8),
        ),
    )
    # Pause 1 seconds.
    time.sleep(1)

import machine
from machine import Pin
import st7735
import time
import random
import vga1_8x8 as font

spi = machine.SPI(2, baudrate=10000000, polarity=1, phase=1, sck=Pin(32), mosi=Pin(5), miso=Pin(36))
display = st7735.ST7735(spi, 80, 160, reset=machine.Pin(33, machine.Pin.OUT), dc=machine.Pin(4, machine.Pin.OUT))
display.init()
    
while True:
    for rotation in range(4):
        display.rotation(rotation)
        display.fill(0)
        col_max = display.width() - font.WIDTH*6
        row_max = display.height() - font.HEIGHT

        for _ in range(250):
            color1 = st7735.color565(random.getrandbits(8), random.getrandbits(8), random.getrandbits(8))
            color2 = st7735.color565(random.getrandbits(8), random.getrandbits(8), random.getrandbits(8))
            display.text(font, "Hello!", random.randint(0, col_max), random.randint(0, row_max), color1, color2)