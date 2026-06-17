/**
 * @file    bme280.c
 * @brief   BME280 driver implementation
 *
 * 硬件资源：
 *   - I2C (PB11=SDA, PB10=SCL)
 *   - M0=PG15, M1=PG10, AUX=PD3
 *
 * 由 CubeMX 完成 I2C/GPIO 初始化，model 层提供 I2C 句柄。
 * 功能：
 *   - 初始化
 *   - 读取温湿度
 *   - 读取气压
 *   - 读取所有数据
 */
#include "bme280.h"

#define REG_ID          0xD0
#define REG_RESET       0xE0
#define REG_CTRL_HUM    0xF2
#define REG_CTRL_MEAS   0xF4
#define REG_CONFIG      0xF5
#define REG_DATA        0xF7
#define REG_CALIB       0x88
#define REG_CALIB_HUM   0xE1

static u8 _read(bme280_t* dev, u8 reg, u8* buf, u8 len)
{
    return bme280_port_i2c_read(dev->i2c, dev->addr, reg, buf, len);
}

static u8 _write(bme280_t* dev, u8 reg, u8* buf, u8 len)
{
    return bme280_port_i2c_write(dev->i2c, dev->addr, reg, buf, len);
}

static u8 _read_calib(bme280_t* dev)
{
    u8 buf[26];

    if (_read(dev, REG_CALIB, buf, 26))
        return BME280_ERR_I2C;

    dev->calib.dig_T1 = (u16)buf[1] << 8 | buf[0];
    dev->calib.dig_T2 = (i16)((u16)buf[3] << 8 | buf[2]);
    dev->calib.dig_T3 = (i16)((u16)buf[5] << 8 | buf[4]);
    dev->calib.dig_P1 = (u16)buf[7] << 8 | buf[6];
    dev->calib.dig_P2 = (i16)((u16)buf[9] << 8 | buf[8]);
    dev->calib.dig_P3 = (i16)((u16)buf[11] << 8 | buf[10]);
    dev->calib.dig_P4 = (i16)((u16)buf[13] << 8 | buf[12]);
    dev->calib.dig_P5 = (i16)((u16)buf[15] << 8 | buf[14]);
    dev->calib.dig_P6 = (i16)((u16)buf[17] << 8 | buf[16]);
    dev->calib.dig_P7 = (i16)((u16)buf[19] << 8 | buf[18]);
    dev->calib.dig_P8 = (i16)((u16)buf[21] << 8 | buf[20]);
    dev->calib.dig_P9 = (i16)((u16)buf[23] << 8 | buf[22]);
    dev->calib.dig_H1 = buf[25];

    if (_read(dev, REG_CALIB_HUM, buf, 7))
        return BME280_ERR_I2C;

    dev->calib.dig_H2 = (i16)((u16)buf[1] << 8 | buf[0]);
    dev->calib.dig_H3 = buf[2];
    dev->calib.dig_H4 = (i16)((i16)buf[3] << 4 | (buf[4] & 0x0F));
    dev->calib.dig_H5 = (i16)((i16)buf[5] << 4 | (buf[4] >> 4));
    dev->calib.dig_H6 = (i8)buf[6];

    return BME280_OK;
}

u8 bme280_init(bme280_t* dev)
{
    u8 id;

    if (!dev || !dev->i2c)
        return BME280_ERR_PARAM;

    if (_read(dev, REG_ID, &id, 1))
        return BME280_ERR_I2C;

    if (id != 0x60)
        return BME280_ERR_CHIP_ID;

    if (_read_calib(dev))
        return BME280_ERR_I2C;

    u8 ctrl_hum = 0x01;
    _write(dev, REG_CTRL_HUM, &ctrl_hum, 1);

    u8 ctrl_meas = (0x01 << 5) | (0x01 << 2) | 0x03;
    _write(dev, REG_CTRL_MEAS, &ctrl_meas, 1);

    u8 config = 0x00;
    _write(dev, REG_CONFIG, &config, 1);

    return BME280_OK;
}

u8 bme280_read_all(bme280_t* dev, float* temp_c, float* press_hpa, float* humi_prh)
{
    u8 buf[8];

    if (!dev)
        return BME280_ERR_PARAM;

    if (_read(dev, REG_DATA, buf, 8))
        return BME280_ERR_I2C;

    i32 raw_press = (i32)(((u32)buf[0] << 12) | ((u32)buf[1] << 4) | ((u32)buf[2] >> 4));
    i32 raw_temp  = (i32)(((u32)buf[3] << 12) | ((u32)buf[4] << 4) | ((u32)buf[5] >> 4));
    i32 raw_hum   = (i32)(((u32)buf[6] << 8) | (u32)buf[7]);

    i32 v1 = (((raw_temp >> 3) - ((i32)dev->calib.dig_T1 << 1)) * (i32)dev->calib.dig_T2) >> 11;
    i32 v2 = ((((raw_temp >> 4) - (i32)dev->calib.dig_T1) * ((raw_temp >> 4) - (i32)dev->calib.dig_T1)) >> 12) * (i32)dev->calib.dig_T3 >> 14;
    dev->calib.t_fine = v1 + v2;
    i32 t = (dev->calib.t_fine * 5 + 128) >> 8;
    if (temp_c) *temp_c = (float)t / 100.0f;

    if (press_hpa) {
        i32 v_x1 = (dev->calib.t_fine >> 1) - 64000;
        i32 v_x2 = (((v_x1 >> 2) * (v_x1 >> 2)) >> 11) * (i32)dev->calib.dig_P6;
        v_x2 = v_x2 + ((v_x1 * (i32)dev->calib.dig_P5) << 1);
        v_x2 = (v_x2 >> 2) + ((i32)dev->calib.dig_P4 << 16);
        v_x1 = (((dev->calib.dig_P3 * (((v_x1 >> 2) * (v_x1 >> 2)) >> 13)) >> 3) + (((i32)dev->calib.dig_P2 * v_x1) >> 1)) >> 18;
        v_x1 = ((((32768 + v_x1)) * (i32)dev->calib.dig_P1)) >> 15;

        u32 p = (((u32)(1048576 - raw_press) - (v_x2 >> 12))) * 3125;
        if (p < 0x80000000) {
            if (v_x1)
                p = (p << 1) / (u32)v_x1;
            else
                return BME280_ERR_I2C;
        } else {
            if (v_x1)
                p = (p / (u32)v_x1) * 2;
            else
                return BME280_ERR_I2C;
        }

        v_x1 = ((i32)dev->calib.dig_P9 * (i32)((p >> 3) * (p >> 3) >> 13)) >> 12;
        v_x2 = ((i32)(p >> 2) * (i32)dev->calib.dig_P8) >> 13;
        p = (u32)((i32)p + ((v_x1 + v_x2 + dev->calib.dig_P7) >> 4));
        *press_hpa = (float)p / 100.0f;
    }

    if (humi_prh) {
        i32 v_x1_u32 = dev->calib.t_fine - 76800;
        v_x1_u32 = (((((raw_hum << 14) - ((i32)dev->calib.dig_H4 << 20) - ((i32)dev->calib.dig_H5 * v_x1_u32)) + 16384) >> 15)
                    * (((((((v_x1_u32 * (i32)dev->calib.dig_H6) >> 10) * (((v_x1_u32 * (i32)dev->calib.dig_H3) >> 11) + 32768)) >> 10) + 2097152) * (i32)dev->calib.dig_H2 + 8192) >> 14));
        v_x1_u32 = v_x1_u32 - (((((v_x1_u32 >> 15) * (v_x1_u32 >> 15)) >> 7) * (i32)dev->calib.dig_H1) >> 4);
        v_x1_u32 = (v_x1_u32 < 0) ? 0 : v_x1_u32;
        v_x1_u32 = (v_x1_u32 > 419430400) ? 419430400 : v_x1_u32;
        *humi_prh = (float)(v_x1_u32 >> 12) / 1024.0f;
    }

    return BME280_OK;
}
