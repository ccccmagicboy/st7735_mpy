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
import framebuf

# Subclassing FrameBuffer provides support for graphics primitives
# http://docs.micropython.org/en/latest/pyboard/library/framebuf.html
fonts= {
    0xe585b3:
    [0x10, 0x08, 0x08, 0x00, 0x3F, 0x01, 0x01, 0x01, 0xFF, 0x01, 0x02, 0x02, 0x04, 0x08, 0x30, 0xC0,
     0x10, 0x10, 0x20, 0x00, 0xF8, 0x00, 0x00, 0x00, 0xFE, 0x00, 0x80, 0x80, 0x40, 0x20, 0x18, 0x06],  # 关",0
    0xe788b1:
    [0x00, 0x01, 0x7E, 0x22, 0x11, 0x7F, 0x42, 0x82, 0x7F, 0x04, 0x07, 0x0A, 0x11, 0x20, 0x43, 0x1C,
     0x08, 0xFC, 0x10, 0x10, 0x20, 0xFE, 0x02, 0x04, 0xF8, 0x00, 0xF0, 0x10, 0x20, 0xC0, 0x30, 0x0E],  # 爱",1
    0xe58d95:
    [0x10, 0x08, 0x04, 0x3F, 0x21, 0x21, 0x3F, 0x21, 0x21, 0x3F, 0x01, 0x01, 0xFF, 0x01, 0x01, 0x01,
     0x10, 0x20, 0x40, 0xF8, 0x08, 0x08, 0xF8, 0x08, 0x08, 0xF8, 0x00, 0x00, 0xFE, 0x00, 0x00, 0x00],  # 单",2
    0xe8baab:
    [0x02, 0x04, 0x1F, 0x10, 0x1F, 0x10, 0x1F, 0x10, 0x10, 0x7F, 0x00, 0x00, 0x03, 0x1C, 0xE0, 0x00,
     0x00, 0x00, 0xF0, 0x10, 0xF0, 0x10, 0xF2, 0x14, 0x18, 0xF0, 0x50, 0x90, 0x10, 0x10, 0x50, 0x20],  # 身",3
    0xe78b97:
    [0x00, 0x44, 0x29, 0x11, 0x2A, 0x4C, 0x89, 0x09, 0x19, 0x29, 0x49, 0x89, 0x08, 0x08, 0x50, 0x20,
     0x80, 0x80, 0x00, 0xFC, 0x04, 0x04, 0xE4, 0x24, 0x24, 0x24, 0xE4, 0x24, 0x04, 0x04, 0x28, 0x10],  # 狗",4
    0xe68890:
    [0x00, 0x00, 0x00, 0x3F, 0x20, 0x20, 0x20, 0x3E, 0x22, 0x22, 0x22, 0x22, 0x2A, 0x44, 0x40, 0x81,
     0x50, 0x48, 0x40, 0xFE, 0x40, 0x40, 0x44, 0x44, 0x44, 0x28, 0x28, 0x12, 0x32, 0x4A, 0x86, 0x02],  # 成",5
    0xe995bf:
    [0x08, 0x08, 0x08, 0x08, 0x08, 0x09, 0x08, 0xFF, 0x0A, 0x09, 0x08, 0x08, 0x09, 0x0A, 0x0C, 0x08,
     0x00, 0x10, 0x20, 0x40, 0x80, 0x00, 0x00, 0xFE, 0x00, 0x00, 0x80, 0x40, 0x20, 0x18, 0x06, 0x00],  # 长",6
    0xe58d8f:
    [0x20, 0x20, 0x20, 0x20, 0xFB, 0x20, 0x20, 0x22, 0x22, 0x24, 0x28, 0x20, 0x21, 0x21, 0x22, 0x24,
     0x80, 0x80, 0x80, 0x80, 0xF0, 0x90, 0x90, 0x98, 0x94, 0x92, 0x92, 0x90, 0x10, 0x10, 0x50, 0x20],  # 协",7
    0xe4bc9a:
    [0x01, 0x01, 0x02, 0x04, 0x08, 0x30, 0xCF, 0x00, 0x00, 0x7F, 0x02, 0x04, 0x08, 0x10, 0x3F, 0x10,
     0x00, 0x00, 0x80, 0x40, 0x20, 0x18, 0xE6, 0x00, 0x00, 0xFC, 0x00, 0x00, 0x20, 0x10, 0xF8, 0x08],  # 会",8

    ###
    0xe4b8ad:
    [0x01,0x01,0x01,0x01,0x3F,0x21,0x21,0x21,0x21,0x21,0x3F,0x21,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0xF8,0x08,0x08,0x08,0x08,0x08,0xF8,0x08,0x00,0x00,0x00,0x00],#中0
    
    0xe59bbd:
    [0x00,0x7F,0x40,0x40,0x5F,0x41,0x41,0x4F,0x41,0x41,0x41,0x5F,0x40,0x40,0x7F,0x40,0x00,0xFC,0x04,0x04,0xF4,0x04,0x04,0xE4,0x04,0x44,0x24,0xF4,0x04,0x04,0xFC,0x04],#国1

    0xe4baba:
    [0x01,0x01,0x01,0x01,0x01,0x01,0x02,0x02,0x04,0x04,0x08,0x08,0x10,0x20,0x40,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x80,0x40,0x40,0x20,0x20,0x10,0x08,0x04,0x02],#人2

    ###

     
}

class st7735_pic_fb(framebuf.FrameBuffer):
    def __init__(self, display, width = 160, height = 80):
        self.width = width
        self.height = height
        self.buffer = bytearray(self.width * self.height * 2)
        super().__init__(self.buffer, self.width, self.height, framebuf.RGB565)
        self.init_display()

    def init_display(self):
        self.fill(0)
        self.show(0, 0)
        
    def load(self, buf):
        self.buffer[:] = buf #copy the buf

    def show(self, x, y):
        display.blit_buffer(self.buffer, x, y, self.width, self.height)
        
    def chinese(self, ch_str, x_axis, y_axis):
        offset_ = 0
        y_axis = y_axis*8  # 中文高度一行占8个  
        x_axis = x_axis*16  # 中文宽度占16个
        for k in ch_str: 
            code = 0x00  # 将中文转成16进制编码 
            data_code = k.encode("utf-8") 
            code |= data_code[0] << 16 
            code |= data_code[1] << 8
            code |= data_code[2]
            byte_data = fonts[code]
            for y in range(0, 16):
                a_ = bin(byte_data[y]).replace('0b', '')
                while len(a_) < 8:
                    a_ = '0'+a_
                b_ = bin(byte_data[y+16]).replace('0b', '')
                while len(b_) < 8:
                    b_ = '0'+b_
                for x in range(0, 8):
                    self.pixel(x_axis+offset_+x, y+y_axis, int(a_[x]))   
                    self.pixel(x_axis+offset_+x+8, y+y_axis, int(b_[x]))   
            offset_ += 16        

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

def display_init():
    global display
    spi = machine.SPI(1, baudrate=30000000, polarity=0, phase=0, sck=Pin(32), mosi=Pin(5))# 26.6MHz
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
    
    s = u'中国人'
    st = s.encode('unicode_escape')
    print(st)
    
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
    
def test_scroll():
    global display
    x = 0
    for x in range(163):
        display.vscsad(x)
        time.sleep_ms(10)
    
def test_pic():
    start = time.ticks_us()
    global display
    buf = bytearray(160*80*2)
    during0 = time.ticks_diff(time.ticks_us(), start)
    print('init var: {0:0.3f} ms'.format(during0/1000))        
    with open('/sd/BEEB_TEST.bin', 'rb') as ff:
        buf = ff.read()
    during1 = time.ticks_diff(time.ticks_us(), start)
    print('read from sdnand: {0:0.3f} ms'.format((during1 - during0)/1000))        
    display.blit_buffer(buf, 0, 0, 160, 80)
    during2 = time.ticks_diff(time.ticks_us(), start)
    print('send to lcd: {0:0.3f} ms'.format((during2 - during1)/1000))
    
def test_framebuf():
    global display
    buf = bytearray(160*80*2)
    with open('/sd/BEEB_TEST.bin', 'rb') as ff:
        buf = ff.read()
        
    page0 = st7735_pic_fb(display)
    page0.init_display()
    page0.load(buf)
    
    for x in range(80): 
        # page0.text('World!!!', random.randint(0, 160), random.randint(0, 80))
        time.sleep_ms(10)
        page0.scroll(1, 1)
        page0.show()
        
    page0.init_display()
    page0.chinese('关爱单身狗成长协会中', 0, 0)
    page0.show()
    
def load_bmp_file(file, x, y):
    global display
    with open(file, 'rb') as f:
        if f.read(2) == b'BM':  #header
            dummy = f.read(8) #file size(4), creator bytes(4)
            offset = int.from_bytes(f.read(4), 'little')
            hdrsize = int.from_bytes(f.read(4), 'little')
            width = int.from_bytes(f.read(4), 'little')
            height = int.from_bytes(f.read(4), 'little')
            if int.from_bytes(f.read(2), 'little') == 1: #planes must be 1
                depth = int.from_bytes(f.read(2), 'little')
                if depth == 24 and int.from_bytes(f.read(4), 'little') == 0:#compress method == uncompressed
                    print("Image size: w{} x h{}".format(width, height))
                    rowsize = (width * 3 + 3) & ~3
                    if height < 0:
                        height = -height
                        flip = False
                    else:
                        flip = True
                    w, h = width, height
                    display.set_window(x, x + w - 1, y, y + h - 1)
                    for row in range(h):
                        if flip:
                            pos = offset + (height - 1 - row) * rowsize
                        else:
                            pos = offset + row * rowsize
                        if f.tell() != pos:
                            dummy = f.seek(pos)
                        for col in range(w):
                            bgr = f.read(3)
                            display.push_rgb_color(bgr[2], bgr[1], bgr[0])
                else:
                    print('not 24bit bmp.')
                            
def test_load_bmp_file():
    load_bmp_file('/sd/qq_logo_24bit.bmp', 0, 0)
    
def test_framebuf_pic0():
    global display  
    
    with open('/sd/test0.bmp', 'rb') as f:
        if f.read(2) == b'BM':  #header
            dummy = f.read(8) #file size(4), creator bytes(4)
            offset = int.from_bytes(f.read(4), 'little')
            hdrsize = int.from_bytes(f.read(4), 'little')
            width = int.from_bytes(f.read(4), 'little')
            height = int.from_bytes(f.read(4), 'little')
            buf = bytearray(width*height*2) #init buf
            page0 = st7735_pic_fb(display, width, height)
            page0.init_display()  
            if int.from_bytes(f.read(2), 'little') == 1: #planes must be 1
                depth = int.from_bytes(f.read(2), 'little')
                if depth == 24 and int.from_bytes(f.read(4), 'little') == 0:#compress method == uncompressed
                    print("Image size: w{} x h{}".format(width, height))
                    rowsize = (width * 3 + 3) & ~3
                    print('rowsize is {}'.format(rowsize))
                    if height < 0:
                        height = -height
                        flip = False
                    else:
                        flip = True
                    w, h = width, height
                    
                    for row in reversed(range(h)):
                        if flip:
                            pos = offset + (height - 1 - row) * rowsize
                        else:
                            pos = offset + row * rowsize
                        if f.tell() != pos:
                            dummy = f.seek(pos)
                        for col in range(w):
                            bgr = f.read(3)
                            rgb_color = st7735.color565(bgr[2], bgr[1], bgr[0])
                            buf[row*w*2 + col*2] = rgb_color >> 8
                            buf[row*w*2 + col*2 + 1] = rgb_color
                else:
                    print('not 24bit bmp.')
            page0.load(buf)
            # page0.show(0, 0)
            # while 1:
                # page0.show(random.randint(0, 160), random.randint(0, 80))            

    with open('/sd/skull-and-crossbones.bmp', 'rb') as f:
        if f.read(2) == b'BM':  #header
            dummy = f.read(8) #file size(4), creator bytes(4)
            offset = int.from_bytes(f.read(4), 'little')
            hdrsize = int.from_bytes(f.read(4), 'little')
            width = int.from_bytes(f.read(4), 'little')
            height = int.from_bytes(f.read(4), 'little')
            buf = bytearray(width*height*2) #init buf
            page1 = st7735_pic_fb(display, width, height)
            page1.init_display()  
            if int.from_bytes(f.read(2), 'little') == 1: #planes must be 1
                depth = int.from_bytes(f.read(2), 'little')
                if depth == 24 and int.from_bytes(f.read(4), 'little') == 0:#compress method == uncompressed
                    print("Image size: w{} x h{}".format(width, height))
                    rowsize = (width * 3 + 3) & ~3
                    print('rowsize is {}'.format(rowsize))
                    if height < 0:
                        height = -height
                        flip = False
                    else:
                        flip = True
                    w, h = width, height
                    
                    for row in reversed(range(h)):
                        if flip:
                            pos = offset + (height - 1 - row) * rowsize
                        else:
                            pos = offset + row * rowsize
                        if f.tell() != pos:
                            dummy = f.seek(pos)
                        for col in range(w):
                            bgr = f.read(3)
                            rgb_color = st7735.color565(bgr[2], bgr[1], bgr[0])
                            buf[row*w*2 + col*2] = rgb_color >> 8
                            buf[row*w*2 + col*2 + 1] = rgb_color
                else:
                    print('not 24bit bmp.')
            page1.load(buf)
            page0.blit(page1, 10, 10)

            while 1:
                page0.blit(page1, random.randint(0, 160), random.randint(0, 80), st7735.WHITE)
                page0.show(0, 0)
                time.sleep_ms(100)