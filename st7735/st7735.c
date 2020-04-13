/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ivan Belokobylskiy
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#define __ST7735_VERSION__  "0.1.4"

#include "py/obj.h"
#include "py/objmodule.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include "py/mphal.h"
#include "extmod/machine_spi.h"
#include "st7735.h"
#include <math.h>

#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#define _swap_bytes(val) ( (((val)>>8)&0x00FF)|(((val)<<8)&0xFF00) )

#define ABS(N) (((N)<0)?(-(N)):(N))
#define mp_hal_delay_ms(delay)  (mp_hal_delay_us(delay * 1000))

#define CS_LOW()     { if(self->cs) {mp_hal_pin_write(self->cs, 0);} }
#define CS_HIGH()    { if(self->cs) {mp_hal_pin_write(self->cs, 1);} }
#define DC_LOW()     (mp_hal_pin_write(self->dc, 0))
#define DC_HIGH()    (mp_hal_pin_write(self->dc, 1))
#define RESET_LOW()  { if(self->reset) {mp_hal_pin_write(self->reset, 0);} }
#define RESET_HIGH() { if(self->reset) {mp_hal_pin_write(self->reset, 1);} }


STATIC void write_spi(mp_obj_base_t *spi_obj, const uint8_t *buf, int len) {
    mp_machine_spi_p_t *spi_p = (mp_machine_spi_p_t*)spi_obj->type->protocol;
    spi_p->transfer(spi_obj, len, buf, NULL);
}

// this is the actual C-structure for our new object
typedef struct _st7735_ST7735_obj_t {
    mp_obj_base_t base;

    mp_obj_base_t *spi_obj;
    uint8_t display_width;      // physical width
    uint8_t width;              // logical width (after rotation)
    uint8_t display_height;     // physical width
    uint8_t height;             // logical height (after rotation)
    uint8_t xstart;
    uint8_t ystart;
    uint8_t rotation;
    mp_hal_pin_obj_t reset;
    mp_hal_pin_obj_t dc;
    mp_hal_pin_obj_t cs;
    mp_hal_pin_obj_t backlight;
} st7735_ST7735_obj_t;


// just a definition
mp_obj_t st7735_ST7735_make_new( const mp_obj_type_t *type,
                                  size_t n_args,
                                  size_t n_kw,
                                  const mp_obj_t *args );
STATIC void st7735_ST7735_print( const mp_print_t *print,
                                  mp_obj_t self_in,
                                  mp_print_kind_t kind ) {
    (void)kind;
    st7735_ST7735_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "<ST7735 width=%u, height=%u, spi=%p>", self->width, self->height, self->spi_obj);
}

/* methods start */

STATIC void write_cmd(st7735_ST7735_obj_t *self, uint8_t cmd, const uint8_t *data, int len) {
    CS_LOW()
    if (cmd) {
        DC_LOW();
        write_spi(self->spi_obj, &cmd, 1);
    }
    if (len > 0) {
        DC_HIGH();
        write_spi(self->spi_obj, data, len);
    }
    CS_HIGH()
}

STATIC void set_window(st7735_ST7735_obj_t *self, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    if (x0 > x1 || x1 >= self->width) {
        return;
    }
    if (y0 > y1 || y1 >= self->height) {
        return;
    }
    uint8_t bufx[4] = {(x0+self->xstart) >> 8, (x0+self->xstart) & 0xFF, (x1+self->xstart) >> 8, (x1+self->xstart) & 0xFF};
    uint8_t bufy[4] = {(y0+self->ystart) >> 8, (y0+self->ystart) & 0xFF, (y1+self->ystart) >> 8, (y1+self->ystart) & 0xFF};
    write_cmd(self, ST7735_CASET, bufx, 4);
    write_cmd(self, ST7735_RASET, bufy, 4);
    write_cmd(self, ST7735_RAMWR, NULL, 0);
}

STATIC void fill_color_buffer(mp_obj_base_t* spi_obj, uint16_t color, int length) {
    const int buffer_pixel_size = 128;
    int chunks = length / buffer_pixel_size;
    int rest = length % buffer_pixel_size;
    uint16_t color_swapped = _swap_bytes(color);
    uint16_t buffer[buffer_pixel_size]; // 128 pixels

    // fill buffer with color data
    for (int i = 0; i < length && i < buffer_pixel_size; i++) {
        buffer[i] = color_swapped;
    }
    if (chunks) {
        for (int j = 0; j < chunks; j ++) {
            write_spi(spi_obj, (uint8_t *)buffer, buffer_pixel_size*2);
        }
    }
    if (rest) {
        write_spi(spi_obj, (uint8_t *)buffer, rest*2);
    }
}


STATIC void draw_pixel(st7735_ST7735_obj_t *self, uint8_t x, uint8_t y, uint16_t color) {
    uint8_t hi = color >> 8, lo = color;
    set_window(self, x, y, x, y);
    DC_HIGH();
    CS_LOW();
    write_spi(self->spi_obj, &hi, 1);
    write_spi(self->spi_obj, &lo, 1);
    CS_HIGH();
}


STATIC void fast_hline(st7735_ST7735_obj_t *self, uint8_t x, uint8_t y, uint8_t _w, uint16_t color) {

    int w;

    if (x+_w > self->width)
        w = self->width - x;
    else
        w = _w;

    if (w>0) {
        set_window(self, x, y, x + w - 1, y);
        DC_HIGH();
        CS_LOW();
        fill_color_buffer(self->spi_obj, color, w);
        CS_HIGH();
    }
}

STATIC void fast_vline(st7735_ST7735_obj_t *self, uint8_t x, uint8_t y, uint16_t w, uint16_t color) {
    set_window(self, x, y, x, y + w - 1);
    DC_HIGH();
    CS_LOW();
    fill_color_buffer(self->spi_obj, color, w);
    CS_HIGH();
}


STATIC mp_obj_t st7735_ST7735_hard_reset(mp_obj_t self_in) {
    st7735_ST7735_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    DC_LOW();
    CS_LOW();
    RESET_HIGH();
    mp_hal_delay_ms(10);
    RESET_LOW();
    mp_hal_delay_ms(10);
    RESET_HIGH();
    mp_hal_delay_ms(10);
    CS_HIGH();
    return mp_const_none;
}


STATIC mp_obj_t st7735_ST7735_soft_reset(mp_obj_t self_in) {
    st7735_ST7735_obj_t *self = MP_OBJ_TO_PTR(self_in);

    write_cmd(self, ST7735_SWRESET, NULL, 0);
    mp_hal_delay_ms(10);
    return mp_const_none;
}
/////////////////////////////////////////basic functions above//////////////////////////////////////////////
/////////////////////////////////////////export functions below//////////////////////////////////////////////
// do not expose extra method to reduce size
#ifdef EXPOSE_EXTRA_METHODS
STATIC mp_obj_t st7735_ST7735_write(mp_obj_t self_in, mp_obj_t command, mp_obj_t data) {
    st7735_ST7735_obj_t *self = MP_OBJ_TO_PTR(self_in);

    mp_buffer_info_t src;
    if (data == mp_const_none) {
        write_cmd(self, (uint8_t)mp_obj_get_int(command), NULL, 0);
    } else {
        mp_get_buffer_raise(data, &src, MP_BUFFER_READ);
        write_cmd(self, (uint8_t)mp_obj_get_int(command), (const uint8_t*)src.buf, src.len);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_3(st7735_ST7735_write_obj, st7735_ST7735_write);

MP_DEFINE_CONST_FUN_OBJ_1(st7735_ST7735_hard_reset_obj, st7735_ST7735_hard_reset);
MP_DEFINE_CONST_FUN_OBJ_1(st7735_ST7735_soft_reset_obj, st7735_ST7735_soft_reset);

STATIC mp_obj_t st7735_ST7735_sleep_mode(mp_obj_t self_in, mp_obj_t value) {
    st7735_ST7735_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if(mp_obj_is_true(value)) {
        write_cmd(self, ST7735_SLPIN, NULL, 0);
    } else {
        write_cmd(self, ST7735_SLPOUT, NULL, 0);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(st7735_ST7735_sleep_mode_obj, st7735_ST7735_sleep_mode);

STATIC mp_obj_t st7735_ST7735_set_window(size_t n_args, const mp_obj_t *args) {
    st7735_ST7735_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x0 = mp_obj_get_int(args[1]);
    mp_int_t x1 = mp_obj_get_int(args[2]);
    mp_int_t y0 = mp_obj_get_int(args[3]);
    mp_int_t y1 = mp_obj_get_int(args[4]);

    set_window(self, x0, y0, x1, y1);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(st7735_ST7735_set_window_obj, 5, 5, st7735_ST7735_set_window);

#endif

STATIC mp_obj_t st7735_ST7735_circle(size_t n_args, const mp_obj_t *args) {
    // extract arguments
    st7735_ST7735_obj_t *self   = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x0                 = mp_obj_get_int(args[1]);
    mp_int_t y0                 = mp_obj_get_int(args[2]);
    mp_int_t r                  = mp_obj_get_int(args[3]);

    mp_int_t color;
    
    int xend, rsq;
    int y;
    int xp; 
    int yp;
    int xn; 
    int yn;
    int xyp;
    int yxp;
    int xyn;
    int yxn;

    if (n_args > 4)
        color = _swap_bytes(mp_obj_get_int(args[4]));
    else
        color = _swap_bytes(WHITE); //default color
    
    xend = (int)(0.7071 * r) + 1;
    rsq = r * r;
    
    for(int x=0;x<xend;x++)
    {
        y = (int)(sqrt(rsq - x * x));
        xp = x0 + x;
        yp = y0 + y;
        xn = x0 - x;
        yn = y0 - y;
        xyp = x0 + y;
        yxp = y0 + x;
        xyn = x0 - y;
        yxn = y0 - x;       
        draw_pixel(self, xp, yp, color);
        draw_pixel(self, xp, yn, color);
        draw_pixel(self, xn, yp, color);
        draw_pixel(self, xn, yn, color);
        draw_pixel(self, xyp, yxp, color);
        draw_pixel(self, xyp, yxn, color);
        draw_pixel(self, xyn, yxp, color);
        draw_pixel(self, xyn, yxn, color);       
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(st7735_ST7735_circle_obj, 5, 5, st7735_ST7735_circle);

STATIC mp_obj_t st7735_ST7735_inversion_mode(mp_obj_t self_in, mp_obj_t value) {
    st7735_ST7735_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if(mp_obj_is_true(value)) {
        write_cmd(self, ST7735_INVON, NULL, 0);
    } else {
        write_cmd(self, ST7735_INVOFF, NULL, 0);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(st7735_ST7735_inversion_mode_obj, st7735_ST7735_inversion_mode);


STATIC mp_obj_t st7735_ST7735_fill_rect(size_t n_args, const mp_obj_t *args) {
    st7735_ST7735_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    mp_int_t w = mp_obj_get_int(args[3]);
    mp_int_t h = mp_obj_get_int(args[4]);
    mp_int_t color = mp_obj_get_int(args[5]);

    set_window(self, x, y, x + w - 1, y + h - 1);
    DC_HIGH();
    CS_LOW();
    fill_color_buffer(self->spi_obj, color, w * h);
    CS_HIGH();

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(st7735_ST7735_fill_rect_obj, 6, 6, st7735_ST7735_fill_rect);


STATIC mp_obj_t st7735_ST7735_fill(mp_obj_t self_in, mp_obj_t _color) {
    st7735_ST7735_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t color = mp_obj_get_int(_color);

    set_window(self, 0, 0, self->width - 1, self->height - 1);
    DC_HIGH();
    CS_LOW();
    fill_color_buffer(self->spi_obj, color, self->width * self->height);
    CS_HIGH();

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(st7735_ST7735_fill_obj, st7735_ST7735_fill);


STATIC mp_obj_t st7735_ST7735_pixel(size_t n_args, const mp_obj_t *args) {
    st7735_ST7735_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    mp_int_t color = mp_obj_get_int(args[3]);

    draw_pixel(self, x, y, color);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(st7735_ST7735_pixel_obj, 4, 4, st7735_ST7735_pixel);

STATIC mp_obj_t st7735_ST7735_line(size_t n_args, const mp_obj_t *args) {
    st7735_ST7735_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x0 = mp_obj_get_int(args[1]);
    mp_int_t y0 = mp_obj_get_int(args[2]);
    mp_int_t x1 = mp_obj_get_int(args[3]);
    mp_int_t y1 = mp_obj_get_int(args[4]);
    mp_int_t color = mp_obj_get_int(args[5]);

    bool steep = ABS(y1 - y0) > ABS(x1 - x0);
    if (steep) {
        _swap_int16_t(x0, y0);
        _swap_int16_t(x1, y1);
    }

    if (x0 > x1) {
        _swap_int16_t(x0, x1);
        _swap_int16_t(y0, y1);
    }

    int16_t dx = x1 - x0, dy = ABS(y1 - y0);
    int16_t err = dx >> 1, ystep = -1, xs = x0, dlen = 0;

    if (y0 < y1) ystep = 1;

    // Split into steep and not steep for FastH/V separation
    if (steep) {
        for (; x0 <= x1; x0++) {
        dlen++;
        err -= dy;
        if (err < 0) {
            err += dx;
            if (dlen == 1) draw_pixel(self, y0, xs, color);
            else fast_vline(self, y0, xs, dlen, color);
            dlen = 0; y0 += ystep; xs = x0 + 1;
        }
        }
        if (dlen) fast_vline(self, y0, xs, dlen, color);
    }
    else
    {
        for (; x0 <= x1; x0++) {
        dlen++;
        err -= dy;
        if (err < 0) {
            err += dx;
            if (dlen == 1) draw_pixel(self, xs, y0, color);
            else fast_hline(self, xs, y0, dlen, color);
            dlen = 0; y0 += ystep; xs = x0 + 1;
        }
        }
        if (dlen) fast_hline(self, xs, y0, dlen, color);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(st7735_ST7735_line_obj, 6, 6, st7735_ST7735_line);


STATIC mp_obj_t st7735_ST7735_blit_buffer(size_t n_args, const mp_obj_t *args) {
    st7735_ST7735_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_buffer_info_t buf_info;
    mp_get_buffer_raise(args[1], &buf_info, MP_BUFFER_READ);
    mp_int_t x = mp_obj_get_int(args[2]);
    mp_int_t y = mp_obj_get_int(args[3]);
    mp_int_t w = mp_obj_get_int(args[4]);
    mp_int_t h = mp_obj_get_int(args[5]);

    set_window(self, x, y, x + w - 1, y + h - 1);
    DC_HIGH();
    CS_LOW();

    const int buf_size = 256;
    int limit = MIN(buf_info.len, w * h * 2);
    int chunks = limit / buf_size;
    int rest = limit % buf_size;
    int i = 0;
    for (; i < chunks; i ++) {
        write_spi(self->spi_obj, (const uint8_t*)buf_info.buf + i*buf_size, buf_size);
    }
    if (rest) {
        write_spi(self->spi_obj, (const uint8_t*)buf_info.buf + i*buf_size, rest);
    }
    CS_HIGH();

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(st7735_ST7735_blit_buffer_obj, 6, 6, st7735_ST7735_blit_buffer);


STATIC mp_obj_t st7735_ST7735_text(size_t n_args, const mp_obj_t *args) {
    // extract arguments
    st7735_ST7735_obj_t *self   = MP_OBJ_TO_PTR(args[0]);
    mp_obj_module_t *font       = MP_OBJ_TO_PTR(args[1]);
    const char *str             = mp_obj_str_get_str(args[2]);
    mp_int_t x0                 = mp_obj_get_int(args[3]);
    mp_int_t y0                 = mp_obj_get_int(args[4]);

    mp_obj_dict_t *dict     = MP_OBJ_TO_PTR(font->globals);
    const uint8_t width     = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_WIDTH)));
    const uint8_t height    = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_HEIGHT)));
    const uint8_t first     = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_FIRST)));
    const uint8_t last      = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_LAST)));

    mp_obj_t font_data_buff = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_FONT));
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(font_data_buff, &bufinfo, MP_BUFFER_READ);
    const uint8_t *font_data = bufinfo.buf;

    mp_int_t fg_color;
    mp_int_t bg_color;

    if (n_args > 5)
        fg_color = _swap_bytes(mp_obj_get_int(args[5]));
    else
        fg_color = _swap_bytes(WHITE);

    if (n_args > 6)
        bg_color = _swap_bytes(mp_obj_get_int(args[6]));
    else
        bg_color = _swap_bytes(BLACK);

    uint8_t wide = width / 8;
    uint16_t buf_size = width * height * 2;
    uint16_t *c_buffer = malloc(buf_size);

    if (c_buffer) {
        uint8_t chr;
        while ((chr = *str++)) {
            if (chr >= first && chr <= last) {
                uint16_t buf_idx = 0;
                uint16_t chr_idx = (chr-first)*(height*wide);
                for (uint8_t line = 0; line < height; line++) {
                    for (uint8_t line_byte = 0; line_byte < wide; line_byte++) {
                        uint8_t chr_data = font_data[chr_idx];
                        for (uint8_t bit = 8; bit; bit--) {
                            if (chr_data >> (bit-1) & 1)
                                c_buffer[buf_idx] = fg_color;
                            else
                                c_buffer[buf_idx] = bg_color;
                            buf_idx++;
                        }
                        chr_idx++;
                    }
                }
                uint16_t x1 = x0+width-1;
                if (x1 < self->width) {
                    set_window(self, x0, y0, x1, y0+height-1);
                    DC_HIGH();
                    CS_LOW();
                    write_spi(self->spi_obj, (uint8_t *) c_buffer, buf_size);
                    CS_HIGH();
                }
                x0 += width;
            }
        }
        free(c_buffer);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(st7735_ST7735_text_obj, 5, 7, st7735_ST7735_text);


STATIC void set_rotation(st7735_ST7735_obj_t *self) {
    uint8_t madctl_value = ST7735_MADCTL_BGR;

    if (self->rotation == 0) {              // Portrait
        self->width = self->display_width;
        self->height = self->display_height;
        self->xstart = ST7735_80x160_XSTART;
        self->ystart = ST7735_80x160_YSTART;
    }
    else if (self->rotation == 1) {         // Landscape
        madctl_value |= ST7735_MADCTL_MX | ST7735_MADCTL_MV;
        self->width = self->display_height;
        self->height = self->display_width;
        self->xstart = ST7735_80x160_YSTART;
        self->ystart = ST7735_80x160_XSTART;
    }
    else if (self->rotation == 2) {        // Inverted Portrait
        madctl_value |= ST7735_MADCTL_MX | ST7735_MADCTL_MY;
        self->width = self->display_width;
        self->height = self->display_height;
        self->xstart = ST7735_80x160_XSTART;
        self->ystart = ST7735_80x160_YSTART;
    }
    else if (self->rotation == 3) {         // Inverted Landscape
        madctl_value |= ST7735_MADCTL_MV | ST7735_MADCTL_MY;
        self->width = self->display_height;
        self->height = self->display_width;
        self->xstart = ST7735_80x160_YSTART;
        self->ystart = ST7735_80x160_XSTART;
    }
    const uint8_t madctl[] = { madctl_value };
    write_cmd(self, ST7735_MADCTL, madctl, 1);
    set_window(self, 0, 0, self->width - 1, self->height - 1);
}


STATIC mp_obj_t st7735_ST7735_rotation(mp_obj_t self_in, mp_obj_t value) {
    st7735_ST7735_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t rotation = mp_obj_get_int(value) % 4;
    self->rotation = rotation;
    set_rotation(self);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(st7735_ST7735_rotation_obj, st7735_ST7735_rotation);


STATIC mp_obj_t st7735_ST7735_width(mp_obj_t self_in) {
    st7735_ST7735_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(self->width);
}

MP_DEFINE_CONST_FUN_OBJ_1(st7735_ST7735_width_obj, st7735_ST7735_width);


STATIC mp_obj_t st7735_ST7735_height(mp_obj_t self_in) {
    st7735_ST7735_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(self->height);
}

MP_DEFINE_CONST_FUN_OBJ_1(st7735_ST7735_height_obj, st7735_ST7735_height);
////////////////////////////////////////////////////////////////////////////////////
STATIC mp_obj_t st7735_ST7735_get_xstart(mp_obj_t self_in) {
    st7735_ST7735_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(self->xstart);
}

MP_DEFINE_CONST_FUN_OBJ_1(st7735_ST7735_get_xstart_obj, st7735_ST7735_get_xstart);

STATIC mp_obj_t st7735_ST7735_get_ystart(mp_obj_t self_in) {
    st7735_ST7735_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(self->ystart);
}

MP_DEFINE_CONST_FUN_OBJ_1(st7735_ST7735_get_ystart_obj, st7735_ST7735_get_ystart);
////////////////////////////////////////////////////////////////////////////////////
STATIC mp_obj_t st7735_ST7735_set_xstart(mp_obj_t self_in, mp_obj_t start) {
    st7735_ST7735_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    mp_int_t temp = mp_obj_get_int(start);
    self->xstart = temp;
    
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(st7735_ST7735_set_xstart_obj, st7735_ST7735_set_xstart);

STATIC mp_obj_t st7735_ST7735_set_ystart(mp_obj_t self_in, mp_obj_t start) {
    st7735_ST7735_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    mp_int_t temp = mp_obj_get_int(start);
    self->ystart = temp;
    
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(st7735_ST7735_set_ystart_obj, st7735_ST7735_set_ystart);
////////////////////////////////////////////////////////////////////////////////////
STATIC mp_obj_t st7735_ST7735_vscrdef(size_t n_args, const mp_obj_t *args) {
    // st7735_ST7735_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    // mp_int_t tfa = mp_obj_get_int(args[1]);
    // mp_int_t vsa = mp_obj_get_int(args[2]);
    // mp_int_t bfa = mp_obj_get_int(args[3]);

    // uint8_t buf[6] = {(tfa) >> 8, (tfa) & 0xFF, (vsa) >> 8, (vsa) & 0xFF, (bfa) >> 8, (bfa) & 0xFF};
    // write_cmd(self, ST7735_VSCRDEF, buf, 6);

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(st7735_ST7735_vscrdef_obj, 4, 4, st7735_ST7735_vscrdef);


STATIC mp_obj_t st7735_ST7735_vscsad(mp_obj_t self_in, mp_obj_t vssa_in) {
    st7735_ST7735_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t vssa = mp_obj_get_int(vssa_in);

    uint8_t buf[2] = {(vssa) >> 8, (vssa) & 0xFF};
    write_cmd(self, ST7735_VSCSAD, buf, 2);

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(st7735_ST7735_vscsad_obj, st7735_ST7735_vscsad);


STATIC mp_obj_t st7735_ST7735_init(mp_obj_t self_in) {
    st7735_ST7735_obj_t *self = MP_OBJ_TO_PTR(self_in);
/////////////////////////////////////////////////////////////////////////////////////////////////////    
    st7735_ST7735_hard_reset(self_in);          //hard reset
    mp_hal_delay_ms(10);
/////////////////////////////////////////////////////////////////////////////////////////////////////    
    st7735_ST7735_soft_reset(self_in);          //soft reset
    mp_hal_delay_ms(10);
/////////////////////////////////////////////////////////////////////////////////////////////////////    
    write_cmd(self, ST7735_SLPOUT, NULL, 0);    //sleep out
    mp_hal_delay_ms(10);
/////////////////////////////////////////////////////////////////////////////////////////////////////
    const uint8_t color_mode[] = {COLOR_MODE_16BIT};
    write_cmd(self, ST7735_COLMOD, color_mode, 1);  //p150, set 16-bit mode
    mp_hal_delay_ms(10);
/////////////////////////////////////////////////////////////////////////////////////////////////////    
    const uint8_t frame_rate_mode[] = {0x01, 0x2C, 0x2D};
    write_cmd(self, ST7735_FRMCTR1, frame_rate_mode, 3);    //p159, Frame rate control.
    write_cmd(self, ST7735_FRMCTR2, frame_rate_mode, 3);    //p160
    const uint8_t frame_rate_mode_[] = {0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D};
    write_cmd(self, ST7735_FRMCTR3, frame_rate_mode_, 6);    //p160
    mp_hal_delay_ms(10);
/////////////////////////////////////////////////////////////////////////////////////////////////////    
    const uint8_t inversion_mode[] = {0x07};
    write_cmd(self, ST7735_INVCTR, inversion_mode, 1);  //p162, Display Inversion Control
    mp_hal_delay_ms(10);
/////////////////////////////////////////////////////////////////////////////////////////////////////
    const uint8_t power_mode1[] = {0xA2, 0x02, 0x84};
    write_cmd(self, ST7735_PWCTR1, power_mode1, 3);  //p163, Power Control 1
    mp_hal_delay_ms(10);
/////////////////////////////////////////////////////////////////////////////////////////////////////    
    const uint8_t power_mode2[] = {0xC5};
    write_cmd(self, ST7735_PWCTR2, power_mode2, 1);  //p165, Power Control 2
    mp_hal_delay_ms(10);
/////////////////////////////////////////////////////////////////////////////////////////////////////    
    const uint8_t power_mode3[] = {0x0A, 0x00};
    write_cmd(self, ST7735_PWCTR3, power_mode3, 2);  //p167, Power Control 3
    mp_hal_delay_ms(10);
/////////////////////////////////////////////////////////////////////////////////////////////////////    
    const uint8_t power_mode4[] = {0x8A, 0x2A};
    write_cmd(self, ST7735_PWCTR4, power_mode4, 2);  //p169, Power Control 4
    mp_hal_delay_ms(10);
/////////////////////////////////////////////////////////////////////////////////////////////////////    
    const uint8_t power_mode5[] = {0x8A, 0xEE};
    write_cmd(self, ST7735_PWCTR5, power_mode5, 2);  //p171, Power Control 5
    mp_hal_delay_ms(10);
/////////////////////////////////////////////////////////////////////////////////////////////////////    
    const uint8_t VMCTR1_mode[] = {0x0E};
    write_cmd(self, ST7735_VMCTR1, VMCTR1_mode, 1);  //p173, VCOM Control 1
    mp_hal_delay_ms(10);
/////////////////////////////////////////////////////////////////////////////////////////////////////
    write_cmd(self, ST7735_INVOFF, NULL, 0);    //P123, Display Inversion Off 
    mp_hal_delay_ms(10);
/////////////////////////////////////////////////////////////////////////////////////////////////////    
    self->xstart = ST7735_80x160_XSTART;
    self->ystart = ST7735_80x160_YSTART;
    set_rotation(self);
    mp_hal_delay_ms(10);
/////////////////////////////////////////////////////////////////////////////////////////////////////    
    write_cmd(self, ST7735_INVON, NULL, 0);   //P124, Display Inversion On
    mp_hal_delay_ms(10);
/////////////////////////////////////////////////////////////////////////////////////////////////////
    uint8_t windowLocData[4] = {0x00, 0x00, 0x00, 0x00};
    windowLocData[0] = 0x00;
    windowLocData[1] = self->xstart;
    windowLocData[2] = 0x00;
    windowLocData[3] = self->width + self->xstart;
    write_cmd(self, ST7735_CASET, windowLocData, 4);  //p128, Column address set.
    mp_hal_delay_ms(10);
/////////////////////////////////////////////////////////////////////////////////////////////////////
    windowLocData[1] = self->ystart;
    windowLocData[3] = self->height + self->ystart;
    write_cmd(self, ST7735_CASET, windowLocData, 4);  //p130, Row address set.
    mp_hal_delay_ms(10);
/////////////////////////////////////////////////////////////////////////////////////////////////////
    const uint8_t dataGMCTRP[] = {0x02, 0x1C, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2D, 0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10};
    write_cmd(self, ST7735_GMCTRP1, dataGMCTRP, 16);  //p182, Gamma (‘+’polarity) Correction Characteristics Setting
    const uint8_t dataGMCTRN[] = {0x03, 0x1D, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D, 0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10};
    write_cmd(self, ST7735_GMCTRN1, dataGMCTRN, 16);  //p184, Gamma (‘-’polarity) Correction Characteristics Setting
    mp_hal_delay_ms(10);
/////////////////////////////////////////////////////////////////////////////////////////////////////        
    write_cmd(self, ST7735_NORON, NULL, 0);     //P122, Normal Display Mode On
    mp_hal_delay_ms(10);    
/////////////////////////////////////////////////////////////////////////////////////////////////////
    const mp_obj_t args[] = {
        self_in,
        mp_obj_new_int(0),
        mp_obj_new_int(0),
        mp_obj_new_int(self->width),
        mp_obj_new_int(self->height),
        mp_obj_new_int(BLACK)
    };
    st7735_ST7735_fill_rect(6, args);
    mp_hal_delay_ms(10);
/////////////////////////////////////////////////////////////////////////////////////////////////////
    if (self->backlight)
        mp_hal_pin_write(self->backlight, 1);
    mp_hal_delay_ms(10);
/////////////////////////////////////////////////////////////////////////////////////////////////////
    write_cmd(self, ST7735_DISPON, NULL, 0);     //P127, Display On
    mp_hal_delay_ms(10);
/////////////////////////////////////////////////////////////////////////////////////////////////////
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(st7735_ST7735_init_obj, st7735_ST7735_init);


STATIC mp_obj_t st7735_ST7735_hline(size_t n_args, const mp_obj_t *args) {
    st7735_ST7735_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    mp_int_t w = mp_obj_get_int(args[3]);
    mp_int_t color = mp_obj_get_int(args[4]);

    fast_hline(self, x, y, w, color);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(st7735_ST7735_hline_obj, 5, 5, st7735_ST7735_hline);


STATIC mp_obj_t st7735_ST7735_vline(size_t n_args, const mp_obj_t *args) {
    st7735_ST7735_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    mp_int_t w = mp_obj_get_int(args[3]);
    mp_int_t color = mp_obj_get_int(args[4]);

    fast_vline(self, x, y, w, color);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(st7735_ST7735_vline_obj, 5, 5, st7735_ST7735_vline);


STATIC mp_obj_t st7735_ST7735_rect(size_t n_args, const mp_obj_t *args) {
    st7735_ST7735_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    mp_int_t w = mp_obj_get_int(args[3]);
    mp_int_t h = mp_obj_get_int(args[4]);
    mp_int_t color = mp_obj_get_int(args[5]);

    fast_hline(self, x, y, w, color);
    fast_vline(self, x, y, h, color);
    fast_hline(self, x, y + h - 1, w, color);
    fast_vline(self, x + w - 1, y, h, color);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(st7735_ST7735_rect_obj, 6, 6, st7735_ST7735_rect);


STATIC const mp_rom_map_elem_t st7735_ST7735_locals_dict_table[] = {
    // Do not expose internal functions to fit iram_0 section
#ifdef EXPOSE_EXTRA_METHODS
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&st7735_ST7735_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_hard_reset), MP_ROM_PTR(&st7735_ST7735_hard_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_soft_reset), MP_ROM_PTR(&st7735_ST7735_soft_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep_mode), MP_ROM_PTR(&st7735_ST7735_sleep_mode_obj) },
    { MP_ROM_QSTR(MP_QSTR_inversion_mode), MP_ROM_PTR(&st7735_ST7735_inversion_mode_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_window), MP_ROM_PTR(&st7735_ST7735_set_window_obj) },
#endif
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&st7735_ST7735_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_pixel), MP_ROM_PTR(&st7735_ST7735_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_line), MP_ROM_PTR(&st7735_ST7735_line_obj) },
    { MP_ROM_QSTR(MP_QSTR_blit_buffer), MP_ROM_PTR(&st7735_ST7735_blit_buffer_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill_rect), MP_ROM_PTR(&st7735_ST7735_fill_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&st7735_ST7735_fill_obj) },
    { MP_ROM_QSTR(MP_QSTR_circle), MP_ROM_PTR(&st7735_ST7735_circle_obj) },     //new
    { MP_ROM_QSTR(MP_QSTR_hline), MP_ROM_PTR(&st7735_ST7735_hline_obj) },
    { MP_ROM_QSTR(MP_QSTR_vline), MP_ROM_PTR(&st7735_ST7735_vline_obj) },
    { MP_ROM_QSTR(MP_QSTR_rect), MP_ROM_PTR(&st7735_ST7735_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_text), MP_ROM_PTR(&st7735_ST7735_text_obj) },
    { MP_ROM_QSTR(MP_QSTR_rotation), MP_ROM_PTR(&st7735_ST7735_rotation_obj) },
    { MP_ROM_QSTR(MP_QSTR_width), MP_ROM_PTR(&st7735_ST7735_width_obj) },
    { MP_ROM_QSTR(MP_QSTR_height), MP_ROM_PTR(&st7735_ST7735_height_obj) },
    { MP_ROM_QSTR(MP_QSTR_vscrdef), MP_ROM_PTR(&st7735_ST7735_vscrdef_obj) },
    { MP_ROM_QSTR(MP_QSTR_vscsad), MP_ROM_PTR(&st7735_ST7735_vscsad_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_xstart), MP_ROM_PTR(&st7735_ST7735_get_xstart_obj) },       //new
    { MP_ROM_QSTR(MP_QSTR_get_ystart), MP_ROM_PTR(&st7735_ST7735_get_ystart_obj) },       //new
    { MP_ROM_QSTR(MP_QSTR_set_xstart), MP_ROM_PTR(&st7735_ST7735_set_xstart_obj) },       //new
    { MP_ROM_QSTR(MP_QSTR_set_ystart), MP_ROM_PTR(&st7735_ST7735_set_ystart_obj) },       //new       
};

STATIC MP_DEFINE_CONST_DICT(st7735_ST7735_locals_dict, st7735_ST7735_locals_dict_table);
/* methods end */


const mp_obj_type_t st7735_ST7735_type = {
    { &mp_type_type },
    .name = MP_QSTR_ST7735,
    .print = st7735_ST7735_print,
    .make_new = st7735_ST7735_make_new,
    .locals_dict = (mp_obj_dict_t*)&st7735_ST7735_locals_dict,
};

mp_obj_t st7735_ST7735_make_new(const mp_obj_type_t *type,
                                size_t n_args,
                                size_t n_kw,
                                const mp_obj_t *all_args ) {
    enum {
        ARG_spi, ARG_width, ARG_height, ARG_reset, ARG_dc, ARG_cs,
        ARG_backlight, ARG_rotation
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_spi, MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_width, MP_ARG_INT | MP_ARG_REQUIRED, {.u_int = 0} },
        { MP_QSTR_height, MP_ARG_INT | MP_ARG_REQUIRED, {.u_int = 0} },
        { MP_QSTR_reset, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_dc, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_cs, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_backlight, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_rotation, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0 } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // create new object
    st7735_ST7735_obj_t *self = m_new_obj(st7735_ST7735_obj_t);
    self->base.type = &st7735_ST7735_type;

    // set parameters
    mp_obj_base_t *spi_obj = (mp_obj_base_t*)MP_OBJ_TO_PTR(args[ARG_spi].u_obj);
    self->spi_obj = spi_obj;
    self->display_width = args[ARG_width].u_int;
    self->width = args[ARG_width].u_int;
    self->display_height = args[ARG_height].u_int;
    self->height = args[ARG_height].u_int;
    self->rotation = args[ARG_rotation].u_int % 4;

    if ((self->display_height != 160 && self->display_height != 160) || (self->display_width != 80  && self->display_width != 80)) {
        mp_raise_ValueError("Unsupported display. Only 80x160 is supported");
    }

    if (args[ARG_dc].u_obj == MP_OBJ_NULL) {
        mp_raise_ValueError("must specify dc pins");
    }

    self->dc = mp_hal_get_pin_obj(args[ARG_dc].u_obj);

    if (args[ARG_reset].u_obj != MP_OBJ_NULL) {
        self->reset = mp_hal_get_pin_obj(args[ARG_reset].u_obj);
    }    
    if (args[ARG_cs].u_obj != MP_OBJ_NULL) {
        self->cs = mp_hal_get_pin_obj(args[ARG_cs].u_obj);
    }
    if (args[ARG_backlight].u_obj != MP_OBJ_NULL) {
        self->backlight = mp_hal_get_pin_obj(args[ARG_backlight].u_obj);
    }

    return MP_OBJ_FROM_PTR(self);
}

STATIC uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
}


STATIC mp_obj_t st7735_color565(mp_obj_t r, mp_obj_t g, mp_obj_t b) {
    return MP_OBJ_NEW_SMALL_INT(color565(
        (uint8_t)mp_obj_get_int(r),
        (uint8_t)mp_obj_get_int(g),
        (uint8_t)mp_obj_get_int(b)
    ));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(st7735_color565_obj, st7735_color565);


STATIC void map_bitarray_to_rgb565(uint8_t const *bitarray, uint8_t *buffer, int length, int width,
                                  uint16_t color, uint16_t bg_color) {
    int row_pos = 0;
    for (int i = 0; i < length; i++) {
        uint8_t byte = bitarray[i];
        for (int bi = 7; bi >= 0; bi--) {
            uint8_t b = byte & (1 << bi);
            uint16_t cur_color = b ? color : bg_color;
            *buffer = (cur_color & 0xff00) >> 8;
            buffer ++;
            *buffer = cur_color & 0xff;
            buffer ++;

            row_pos ++;
            if (row_pos >= width) {
                row_pos = 0;
                break;
            }
        }
    }
}


STATIC mp_obj_t st7735_map_bitarray_to_rgb565(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_bitarray, ARG_buffer, ARG_width, ARG_color, ARG_bg_color };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_bitarray, MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_buffer, MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_width, MP_ARG_INT | MP_ARG_REQUIRED, {.u_int = -1} },
        { MP_QSTR_color, MP_ARG_INT, {.u_int = WHITE} },
        { MP_QSTR_bg_color, MP_ARG_INT, {.u_int = BLACK } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_buffer_info_t bitarray_info;
    mp_buffer_info_t buffer_info;
    mp_get_buffer_raise(args[ARG_bitarray].u_obj, &bitarray_info, MP_BUFFER_READ);
    mp_get_buffer_raise(args[ARG_buffer].u_obj, &buffer_info, MP_BUFFER_WRITE);
    mp_int_t width = args[ARG_width].u_int;
    mp_int_t color = args[ARG_color].u_int;
    mp_int_t bg_color = args[ARG_bg_color].u_int;

    map_bitarray_to_rgb565(bitarray_info.buf, buffer_info.buf, bitarray_info.len, width, color, bg_color);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(st7735_map_bitarray_to_rgb565_obj, 3, st7735_map_bitarray_to_rgb565);


STATIC const mp_map_elem_t st7735_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_st7735) },
    { MP_ROM_QSTR(MP_QSTR_color565), (mp_obj_t)&st7735_color565_obj },
    { MP_ROM_QSTR(MP_QSTR_map_bitarray_to_rgb565), (mp_obj_t)&st7735_map_bitarray_to_rgb565_obj },
    { MP_ROM_QSTR(MP_QSTR_ST7735), (mp_obj_t)&st7735_ST7735_type },
    { MP_ROM_QSTR(MP_QSTR_BLACK), MP_ROM_INT(BLACK) },
    { MP_ROM_QSTR(MP_QSTR_BLUE), MP_ROM_INT(BLUE) },
    { MP_ROM_QSTR(MP_QSTR_RED), MP_ROM_INT(RED) },
    { MP_ROM_QSTR(MP_QSTR_GREEN), MP_ROM_INT(GREEN) },
    { MP_ROM_QSTR(MP_QSTR_CYAN), MP_ROM_INT(CYAN) },
    { MP_ROM_QSTR(MP_QSTR_MAGENTA), MP_ROM_INT(MAGENTA) },
    { MP_ROM_QSTR(MP_QSTR_YELLOW), MP_ROM_INT(YELLOW) },
    { MP_ROM_QSTR(MP_QSTR_WHITE), MP_ROM_INT(WHITE) },
    { MP_ROM_QSTR(MP_QSTR_MAROON), MP_ROM_INT(MAROON) },//NEW
    { MP_ROM_QSTR(MP_QSTR_FOREST), MP_ROM_INT(FOREST) },//NEW
    { MP_ROM_QSTR(MP_QSTR_NAVY), MP_ROM_INT(NAVY) },    //NEW
    { MP_ROM_QSTR(MP_QSTR_PURPLE), MP_ROM_INT(PURPLE) },//NEW
    { MP_ROM_QSTR(MP_QSTR_GRAY), MP_ROM_INT(GRAY) },    //NEW
};

STATIC MP_DEFINE_CONST_DICT (mp_module_st7735_globals, st7735_module_globals_table );

const mp_obj_module_t mp_module_st7735 = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_st7735_globals,
};

MP_REGISTER_MODULE(MP_QSTR_st7735, mp_module_st7735, MODULE_ST7735_ENABLED);
