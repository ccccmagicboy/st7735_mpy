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
import uos
import network

def sd_init():
    global sd
    sd = machine.SDCard(slot=1, width=1, freq=40000000)
    uos.mount(sd, '/sd')
    print(uos.listdir('/sd'))
    
def wifi_init():
    sta_if = network.WLAN(network.STA_IF)
    sta_if.active(True)
    sta_if.connect('TP-LINK_AB2B02', '58930933')
    while True:
        if sta_if.isconnected():
            print(sta_if.ifconfig())
            break
    
def ftp_init():
    import uftpd

def init():
    global display
    spi = machine.SPI(1, baudrate=30000000, polarity=0, phase=0, sck=Pin(32), mosi=Pin(5))
    display = st7735.ST7735(spi, 80, 160, dc=machine.Pin(4, machine.Pin.OUT), rotation=3)
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
            
def random_circle():
    global display
    while True:
        for rotation in range(4):
            display.rotation(rotation)
            display.fill(0)
            col_max = display.width()
            row_max = display.height()

            for _ in range(250):
                color1 = st7735.color565(random.getrandbits(8), random.getrandbits(8), random.getrandbits(8))
                color2 = st7735.color565(random.getrandbits(8), random.getrandbits(8), random.getrandbits(8))
                display.circle(random.randint(0, col_max), random.randint(0, row_max), 10, color1, color2)
                # display.circle(random.randint(0, col_max), random.randint(0, row_max), 15, st7735.WHITE, st7735.BLUE)
                
def circle_test():
    global display
    
    for x in reversed(range(0, 40)):
        color1 = st7735.color565(random.getrandbits(8), random.getrandbits(8), random.getrandbits(8))
        color2 = st7735.color565(random.getrandbits(8), random.getrandbits(8), random.getrandbits(8))
        display.circle(40, 80, x, color1, color2)
        
            
def chinese_font_test():
    global display
    
    for x in range(10):
        display.show_chinese(x*16, 0, 0, 16, st7735.WHITE, st7735.BLACK)
        
    for x in range(10):
        display.show_chinese(x*16, 16, 1, 16, st7735.WHITE, st7735.BLACK)

    for x in range(10):
        display.show_chinese(x*16, 32, 2, 16, st7735.WHITE, st7735.BLACK)

    for x in range(10):
        display.show_chinese(x*16, 48, 3, 16, st7735.WHITE, st7735.BLACK)

    for x in range(10):
        display.show_chinese(x*16, 64, 4, 16, st7735.WHITE, st7735.BLACK)        
                   
def qq_pic():
    global display
    buf = bytearray(0)
    with open('/sd/test.bin', 'rb') as ff:
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
    display.blit_buffer(buf, 0, 0, 40, 40)
    
def test_pic():
    start = time.ticks_us()
    global display
    buf = bytearray(160*80*2)
    with open('/sd/BEEB_TEST.bin', 'rb') as ff:
        buf = ff.read()
    display.blit_buffer(buf, 0, 0, 160, 80)
    during = time.ticks_diff(time.ticks_us(), start)
    
    print('{0:0.3f} ms'.format(during/1000))
    