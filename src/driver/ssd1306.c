#include "ssd1306.h"
#include "ssd1306_font.h"

#define OLED_I2C_ADDRESS 0x78
#define OLED_I2C_TIMEOUT 0x100

static void ssd1306_write_command(ssd1306_t* dev, u8 cmd)
{
    ssd1306_port_i2c_mem_write(dev->i2c, OLED_I2C_ADDRESS, 0x00, 1, &cmd, 1, OLED_I2C_TIMEOUT);
}

static void ssd1306_write_data(ssd1306_t* dev, u8 data)
{
    ssd1306_port_i2c_mem_write(dev->i2c, OLED_I2C_ADDRESS, 0x40, 1, &data, 1, OLED_I2C_TIMEOUT);
}

void ssd1306_init(ssd1306_t* dev)
{
    ssd1306_port_delay_ms(200);

    ssd1306_write_command(dev, 0xAE);                      // Display OFF
    ssd1306_write_command(dev, 0xD5); ssd1306_write_command(dev, 0x80);  // Oscillator freq
    ssd1306_write_command(dev, 0xA8); ssd1306_write_command(dev, 0x3F);  // MUX ratio (64)
    ssd1306_write_command(dev, 0xD3); ssd1306_write_command(dev, 0x00);  // Display offset
    ssd1306_write_command(dev, 0x40);                      // Display start line (0)
    ssd1306_write_command(dev, 0xA1);                      // Segment remap
    ssd1306_write_command(dev, 0xC8);                      // COM scan remap
    ssd1306_write_command(dev, 0xDA); ssd1306_write_command(dev, 0x12);  // COM pins (alt)
    ssd1306_write_command(dev, 0x81); ssd1306_write_command(dev, 0xCF);  // Contrast
    ssd1306_write_command(dev, 0xD9); ssd1306_write_command(dev, 0xF1);  // Pre-charge
    ssd1306_write_command(dev, 0xDB); ssd1306_write_command(dev, 0x40);  // VCOMH
    ssd1306_write_command(dev, 0x20); ssd1306_write_command(dev, 0x00);  // Page addressing mode
    ssd1306_write_command(dev, 0xA4);                      // Resume to RAM
    ssd1306_write_command(dev, 0xA6);                      // Normal display
    ssd1306_write_command(dev, 0x8D); ssd1306_write_command(dev, 0x14);  // Charge pump ON

    ssd1306_port_delay_ms(100);                            // Wait for charge pump to stabilize

    ssd1306_write_command(dev, 0xAF);                      // Display ON
}

void ssd1306_update(ssd1306_t* dev)
{
    for (u8 i = 0; i < 8; i++) {
        ssd1306_write_command(dev, 0xb0 + i);
        ssd1306_write_command(dev, 0x00);
        ssd1306_write_command(dev, 0x10);
        for (u8 n = 0; n < 128; n++)
            ssd1306_write_data(dev, 1);
    }
}

void ssd1306_clear(ssd1306_t* dev)
{
    for (u8 i = 0; i < 8; i++) {
        ssd1306_write_command(dev, 0xb0 + i);
        ssd1306_write_command(dev, 0x00);
        ssd1306_write_command(dev, 0x10);
        for (u8 n = 0; n < 128; n++)
            ssd1306_write_data(dev, 0);
    }
}

void ssd1306_display_on(ssd1306_t* dev)
{
    ssd1306_write_command(dev, 0X8D);
    ssd1306_write_command(dev, 0X14);
    ssd1306_write_command(dev, 0XAF);
}

void ssd1306_display_off(ssd1306_t* dev)
{
    ssd1306_write_command(dev, 0X8D);
    ssd1306_write_command(dev, 0X10);
    ssd1306_write_command(dev, 0XAE);
}

void ssd1306_set_pos(ssd1306_t* dev, u8 x, u8 y)
{
    ssd1306_write_command(dev, 0xb0 + y);
    ssd1306_write_command(dev, ((x & 0xf0) >> 4) | 0x10);
    ssd1306_write_command(dev, x & 0x0f);
}

static u32 ssd1306_pow(u8 m, u8 n)
{
    u32 result = 1;
    while (n--)
        result *= m;
    return result;
}

void ssd1306_show_char(ssd1306_t* dev, u8 x, u8 y, u8 chr, u8 char_size, u8 color_turn)
{
    u8 c = chr - ' ';
    if (x > 128 - 1) {
        x = 0;
        y = y + 2;
    }
    if (char_size == 16) {
        ssd1306_set_pos(dev, x, y);
        for (u8 i = 0; i < 8; i++) {
            if (color_turn)
                ssd1306_write_data(dev, ~F8X16[c * 16 + i]);
            else
                ssd1306_write_data(dev, F8X16[c * 16 + i]);
        }
        ssd1306_set_pos(dev, x, y + 1);
        for (u8 i = 0; i < 8; i++) {
            if (color_turn)
                ssd1306_write_data(dev, ~F8X16[c * 16 + i + 8]);
            else
                ssd1306_write_data(dev, F8X16[c * 16 + i + 8]);
        }
    }
    else {
        ssd1306_set_pos(dev, x, y);
        for (u8 i = 0; i < 6; i++) {
            if (color_turn)
                ssd1306_write_data(dev, ~F6x8[c][i]);
            else
                ssd1306_write_data(dev, F6x8[c][i]);
        }
    }
}

void ssd1306_show_string(ssd1306_t* dev, u8 x, u8 y, char* str, u8 char_size, u8 color_turn)
{
    u8 j = 0;
    while (str[j] != '\0') {
        ssd1306_show_char(dev, x, y, str[j], char_size, color_turn);
        if (char_size == OLED_FONT_6X8)
            x += 6;
        else
            x += 8;

        if (x > 122 && char_size == 12) {
            x = 0;
            y++;
        }
        if (x > 120 && char_size == 16) {
            x = 0;
            y++;
        }
        j++;
    }
}

void ssd1306_show_num(ssd1306_t* dev, u8 x, u8 y, u32 num, u8 len, u8 font_size, u8 color_turn)
{
    u8 enshow = 0;
    for (u8 t = 0; t < len; t++) {
        u8 temp = (num / ssd1306_pow(10, len - t - 1)) % 10;
        if (enshow == 0 && t < (len - 1)) {
            if (temp == 0) {
                ssd1306_show_char(dev, x + (font_size / 2) * t, y, ' ', font_size, color_turn);
                continue;
            }
            else
                enshow = 1;
        }
        ssd1306_show_char(dev, x + (font_size / 2) * t, y, temp + '0', font_size, color_turn);
    }
}

void ssd1306_show_decimal(ssd1306_t* dev, u8 x, u8 y, float num, u8 z_len, u8 f_len,
                          u8 font_size, u8 color_turn)
{
    u8 enshow = 0;
    u8 i = 0;
    if (num < 0) {
        z_len += 1;
        i = 1;
        num = -num;
    }
    int z_temp = (int)num;
    for (u8 t = 0; t < z_len; t++) {
        u8 temp = (z_temp / ssd1306_pow(10, z_len - t - 1)) % 10;
        if (enshow == 0 && t < (z_len - 1)) {
            if (temp == 0) {
                ssd1306_show_char(dev, x + (font_size / 2) * t, y, ' ', font_size, color_turn);
                continue;
            }
            else
                enshow = 1;
        }
        ssd1306_show_char(dev, x + (font_size / 2) * t, y, temp + '0', font_size, color_turn);
    }
    ssd1306_show_char(dev, x + (font_size / 2) * (z_len), y, '.', font_size, color_turn);

    int f_temp = (int)((num - z_temp) * (ssd1306_pow(10, f_len)));
    for (u8 t = 0; t < f_len; t++) {
        u8 temp = (f_temp / ssd1306_pow(10, f_len - t - 1)) % 10;
        ssd1306_show_char(dev, x + (font_size / 2) * (t + z_len) + 5, y, temp + '0', font_size, color_turn);
    }
    if (i == 1)
        ssd1306_show_char(dev, x, y, '-', font_size, color_turn);
}

void ssd1306_draw_bmp(ssd1306_t* dev, u8 x0, u8 y0, u8 x1, u8 y1, u8* bmp, u8 color_turn)
{
    u32 j = 0;
    u8 y;

    if (y1 % 8 == 0)
        y = y1 / 8;
    else
        y = y1 / 8 + 1;
    for (y = y0; y < y1; y++) {
        ssd1306_set_pos(dev, x0, y);
        for (u8 x = x0; x < x1; x++) {
            if (color_turn)
                ssd1306_write_data(dev, ~bmp[j++]);
            else
                ssd1306_write_data(dev, bmp[j++]);
        }
    }
}

void ssd1306_horizontal_shift(ssd1306_t* dev, u8 dir)
{
    ssd1306_write_command(dev, 0x2e);
    ssd1306_write_command(dev, dir);
    ssd1306_write_command(dev, 0x00);
    ssd1306_write_command(dev, 0x00);
    ssd1306_write_command(dev, 0x07);
    ssd1306_write_command(dev, 0x07);
    ssd1306_write_command(dev, 0x00);
    ssd1306_write_command(dev, 0xff);
    ssd1306_write_command(dev, 0x2f);
}

void ssd1306_some_horizontal_shift(ssd1306_t* dev, u8 dir, u8 start_page, u8 end_page)
{
    ssd1306_write_command(dev, 0x2e);
    ssd1306_write_command(dev, dir);
    ssd1306_write_command(dev, 0x00);
    ssd1306_write_command(dev, start_page);
    ssd1306_write_command(dev, 0x07);
    ssd1306_write_command(dev, end_page);
    ssd1306_write_command(dev, 0x00);
    ssd1306_write_command(dev, 0xff);
    ssd1306_write_command(dev, 0x2f);
}

void ssd1306_vertical_and_horizontal_shift(ssd1306_t* dev, u8 dir)
{
    ssd1306_write_command(dev, 0x2e);
    ssd1306_write_command(dev, dir);
    ssd1306_write_command(dev, 0x01);
    ssd1306_write_command(dev, 0x00);
    ssd1306_write_command(dev, 0x07);
    ssd1306_write_command(dev, 0x07);
    ssd1306_write_command(dev, 0x01);
    ssd1306_write_command(dev, 0x00);
    ssd1306_write_command(dev, 0xff);
    ssd1306_write_command(dev, 0x2f);
}

void ssd1306_display_mode(ssd1306_t* dev, u8 mode)
{
    ssd1306_write_command(dev, mode);
}

void ssd1306_intensity_control(ssd1306_t* dev, u8 intensity)
{
    ssd1306_write_command(dev, 0x81);
    ssd1306_write_command(dev, intensity);
}
