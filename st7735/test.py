import machine
from machine import Pin
import st7735
import time
import random
import vga1_8x8 as font
from struct import unpack
from struct import pack
import binascii
import sys

global display

def init():
    global display
    spi = machine.SPI(1, baudrate=30000000, polarity=0, phase=0, sck=Pin(32), mosi=Pin(5))
    display = st7735.ST7735(spi, 80, 160, dc=machine.Pin(4, machine.Pin.OUT))
    display.init()

def random_color_fill():
    global display    
    while True:
        display.fill(
            st7735.color565(
                random.getrandbits(8),
                random.getrandbits(8),
                random.getrandbits(8),
            ),
        )
        # Pause 1 seconds.
        time.sleep_ms(200)
        
def random_text():
    global display
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
            
def random_circle(display, x0, y0, r, color):
    global display
    while True:
        for rotation in range(4):
            display.rotation(rotation)
            display.fill(0)
            col_max = display.width() - font.WIDTH*6
            row_max = display.height() - font.HEIGHT

            for _ in range(250):
                color1 = st7735.color565(random.getrandbits(8), random.getrandbits(8), random.getrandbits(8))
                display.circle(random.randint(0, col_max), random.randint(0, row_max), 5, color1)
            
def qq_pic():
    global display
    buf = bytearray(0)
    with open('test.bin', 'rb') as ff:
        try:
            while True:
                rr = unpack('BB', ff.read(2))
                buf.append(rr[1])
                buf.append(rr[0])
        except Exception as e:
            print(str(e))
            print('finished!')
        else:
            pass

    #print(buf, len(buf))
    display.blit_buffer(buf, 100, 50, 40, 40)