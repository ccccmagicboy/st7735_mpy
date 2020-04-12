#ifndef __ST7735_H__
#define __ST7735_H__

#ifdef __cplusplus
extern "C" {
#endif

#define ST7735_240x240_XSTART 0
#define ST7735_240x240_YSTART 0
#define ST7735_135x240_XSTART 52
#define ST7735_135x240_YSTART 40


// color modes
#define COLOR_MODE_65K      0x50
#define COLOR_MODE_262K     0x60
#define COLOR_MODE_12BIT    0x03
#define COLOR_MODE_16BIT    0x05
#define COLOR_MODE_18BIT    0x06
#define COLOR_MODE_16M      0x07

// COMMAND REGISTER ADDRESS
#define ST7735_NOP     0x00
#define ST7735_SWRESET 0x01
#define ST7735_RDDID   0x04
#define ST7735_RDDST   0x09
#define ST7735_RDDPM   0x0A
#define ST7735_RDDMADCTL   0x0B
#define ST7735_RDDCOLMOD   0x0C
#define ST7735_RDDIM   0x0D
#define ST7735_RDDSM   0x0E
#define ST7735_RDDSDR  0x0F

#define ST7735_SLPIN   0x10
#define ST7735_SLPOUT  0x11
#define ST7735_PTLON   0x12
#define ST7735_NORON   0x13

#define ST7735_INVOFF  0x20
#define ST7735_INVON   0x21
#define ST7735_GAMSET  0x26     //NEW
#define ST7735_DISPOFF 0x28
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_RGBSET  0x2D     //NEW
#define ST7735_RAMRD   0x2E

#define ST7735_PTLAR   0x30
#define ST7735_SCRLAR  0x33     //NEW
#define ST7735_TEOFF   0x34     //NEW
#define ST7735_TEON    0x35     //NEW
#define ST7735_MADCTL  0x36
#define ST7735_VSCSAD  0x37
#define ST7735_IDMOFF  0x38     //NEW
#define ST7735_IDMON   0x39     //NEW
#define ST7735_COLMOD  0x3A

#define ST7735_RDID1   0xDA
#define ST7735_RDID2   0xDB
#define ST7735_RDID3   0xDC

#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR  0xB4

#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5
#define ST7735_VMOFCTR 0xC7

#define ST7735_WRID2 0xD1
#define ST7735_WRID3 0xD2

#define ST7735_NVFCTR1 0xD9
#define ST7735_NVFCTR2 0xDE
#define ST7735_NVFCTR3 0xDF

#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1
#define ST7735_GCV     0xFC

#define ST7735_MADCTL_MY  0x80  // Page Address Order
#define ST7735_MADCTL_MX  0x40  // Column Address Order
#define ST7735_MADCTL_MV  0x20  // Page/Column Order
#define ST7735_MADCTL_ML  0x10  // Line Address Order
#define ST7735_MADCTL_MH  0x04  // Display Data Latch Order
#define ST7735_MADCTL_RGB 0x00
#define ST7735_MADCTL_BGR 0x08

// Color definitions
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define MAROON  0x8000  //NEW
#define FOREST  0x0410  //NEW
#define NAVY    0x0010  //NEW
#define PURPLE  0xF81F  //NEW
#define GRAY    0x8410  //NEW

#ifdef  __cplusplus
}
#endif /*  __cplusplus */

#endif  /*  __ST7735_H__ */
