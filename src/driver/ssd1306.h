/**
 * @file    ssd1306.h
 * @brief   SSD1306 OLED 驱动（I2C 接口）
 *
 * 遵循 motor 驱动层约定：
 *   - port 层函数由 board/ssd1306_port.c 实现（extern 声明，无需绑定步骤）
 *   - I2C 句柄通过 ssd1306_t::i2c 传入，driver 自身不持有全局状态
 */

#ifndef DRIVER_SSD1306_H
#define DRIVER_SSD1306_H

#include "../util/datatype.h"

/* ===== port 层函数（由 board/ssd1306_port.c 实现） ===== */

/** @brief 阻塞延时（ms） */
extern void ssd1306_port_delay_ms(u32 ms);

/**
 * @brief I2C 写寄存器（HAL_I2C_Mem_Write 封装）
 * @param i2c           I2C 句柄（void*）
 * @param dev_addr      设备地址
 * @param mem_addr      内部寄存器/内存地址
 * @param mem_addr_size 地址宽度（1 或 2）
 * @param data          数据缓冲区
 * @param data_size     数据长度
 * @param timeout       超时时间
 */
extern void ssd1306_port_i2c_mem_write(void* i2c, u16 dev_addr, u8 mem_addr, u16 mem_addr_size,
                                       u8* data, u16 data_size, u32 timeout);

/* ===== OLED 设备句柄 ===== */

/**
 * @brief OLED 设备句柄
 */
typedef struct {
    void* i2c;  /**< I2C 硬件句柄（I2C_HandleTypeDef*） */
} ssd1306_t;

/* ===== 字体大小常量 ===== */

#define OLED_FONT_6X8  12  /**< 6x8 像素字体 */
#define OLED_FONT_8X16 16  /**< 8x16 像素字体 */

/* ===== 驱动 API ===== */

/**
 * @brief 初始化 OLED（发送初始化指令序列）
 * @param dev  设备句柄
 */
void ssd1306_init(ssd1306_t* dev);

/**
 * @brief 全屏清除（所有像素置 0）
 * @param dev  设备句柄
 */
void ssd1306_clear(ssd1306_t* dev);

/** @brief 开启显示 */
void ssd1306_display_on(ssd1306_t* dev);

/** @brief 关闭显示 */
void ssd1306_display_off(ssd1306_t* dev);

/**
 * @brief 设置光标位置
 * @param dev  设备句柄
 * @param x    列坐标（0-127）
 * @param y    页坐标（0-7）
 */
void ssd1306_set_pos(ssd1306_t* dev, u8 x, u8 y);

/**
 * @brief 全屏刷新（整屏填充 0x01）
 * @param dev  设备句柄
 */
void ssd1306_update(ssd1306_t* dev);

/**
 * @brief 显示整数
 * @param dev        设备句柄
 * @param x          列坐标
 * @param y          页坐标
 * @param num        待显示的数字
 * @param len        数字位数
 * @param font_size  字体（OLED_FONT_6X8 / OLED_FONT_8X16）
 * @param color_turn 0=正常, 1=反色
 */
void ssd1306_show_num(ssd1306_t* dev, u8 x, u8 y, u32 num, u8 len, u8 font_size, u8 color_turn);

/**
 * @brief 显示浮点数
 * @param dev        设备句柄
 * @param x          列坐标
 * @param y          页坐标
 * @param num        待显示的浮点数
 * @param z_len      整数部分位数
 * @param f_len      小数部分位数
 * @param font_size  字体
 * @param color_turn 0=正常, 1=反色
 */
void ssd1306_show_decimal(ssd1306_t* dev, u8 x, u8 y, float num, u8 z_len, u8 f_len,
                          u8 font_size, u8 color_turn);

/**
 * @brief 显示单个字符
 * @param dev        设备句柄
 * @param x          列坐标
 * @param y          页坐标
 * @param chr        字符 ASCII 码
 * @param char_size  字体
 * @param color_turn 0=正常, 1=反色
 */
void ssd1306_show_char(ssd1306_t* dev, u8 x, u8 y, u8 chr, u8 char_size, u8 color_turn);

/**
 * @brief 显示字符串
 * @param dev        设备句柄
 * @param x          列坐标
 * @param y          页坐标
 * @param str        以 '\\0' 结尾的字符串
 * @param char_size  字体
 * @param color_turn 0=正常, 1=反色
 */
void ssd1306_show_string(ssd1306_t* dev, u8 x, u8 y, char* str, u8 char_size, u8 color_turn);

/**
 * @brief 绘制位图
 * @param dev        设备句柄
 * @param x0, y0     左上角坐标
 * @param x1, y1     右下角坐标
 * @param bmp        位图数据
 * @param color_turn 0=正常, 1=反色
 */
void ssd1306_draw_bmp(ssd1306_t* dev, u8 x0, u8 y0, u8 x1, u8 y1, u8* bmp, u8 color_turn);

/**
 * @brief 水平滚动（整屏）
 * @param dev  设备句柄
 * @param dir  方向（0x26=左, 0x27=右）
 */
void ssd1306_horizontal_shift(ssd1306_t* dev, u8 dir);

/**
 * @brief 水平滚动（指定页范围）
 * @param dev         设备句柄
 * @param dir         方向
 * @param start_page  起始页（0-7）
 * @param end_page    结束页（0-7）
 */
void ssd1306_some_horizontal_shift(ssd1306_t* dev, u8 dir, u8 start_page, u8 end_page);

/**
 * @brief 垂直+水平滚动
 * @param dev  设备句柄
 * @param dir  方向
 */
void ssd1306_vertical_and_horizontal_shift(ssd1306_t* dev, u8 dir);

/**
 * @brief 设置显示模式
 * @param dev   设备句柄
 * @param mode  模式指令
 */
void ssd1306_display_mode(ssd1306_t* dev, u8 mode);

/**
 * @brief 设置对比度
 * @param dev        设备句柄
 * @param intensity  对比度值（0-255）
 */
void ssd1306_intensity_control(ssd1306_t* dev, u8 intensity);

#endif /* DRIVER_SSD1306_H */
