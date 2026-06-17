#ifndef DRIVER_BME280_H
#define DRIVER_BME280_H

#include "../util/datatype.h"

#define BME280_OK           0
#define BME280_ERR_I2C      1
#define BME280_ERR_PARAM    2
#define BME280_ERR_CHIP_ID  3

#define BME280_I2C_ADDR_0   0x76
#define BME280_I2C_ADDR_1   0x77

extern u8  bme280_port_i2c_read(void* i2c, u8 dev_addr, u8 reg_addr, u8* data, u8 len);
extern u8  bme280_port_i2c_write(void* i2c, u8 dev_addr, u8 reg_addr, u8* data, u8 len);
extern void bme280_port_delay_ms(u32 ms);

typedef struct {
    u16 dig_T1;
    i16 dig_T2;
    i16 dig_T3;
    u16 dig_P1;
    i16 dig_P2;
    i16 dig_P3;
    i16 dig_P4;
    i16 dig_P5;
    i16 dig_P6;
    i16 dig_P7;
    i16 dig_P8;
    i16 dig_P9;
    u8  dig_H1;
    i16 dig_H2;
    u8  dig_H3;
    i16 dig_H4;
    i16 dig_H5;
    i8  dig_H6;
    i32 t_fine;
} bme280_calib_t;

typedef struct {
    void*          i2c;
    u8             addr;
    bme280_calib_t calib;
} bme280_t;

u8  bme280_init(bme280_t* dev);
u8  bme280_read_all(bme280_t* dev, float* temp_c, float* press_hpa, float* humi_prh);

#endif
