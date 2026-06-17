#include "mcu_wrapper.h"
#include "../util/datatype.h"

u8 bme280_port_i2c_read(void* i2c, u8 dev_addr, u8 reg_addr, u8* data, u8 len)
{
    return (HAL_I2C_Mem_Read((I2C_HandleTypeDef*)i2c, dev_addr << 1, reg_addr,
                             I2C_MEMADD_SIZE_8BIT, data, len, 100) == HAL_OK) ? 0 : 1;
}

u8 bme280_port_i2c_write(void* i2c, u8 dev_addr, u8 reg_addr, u8* data, u8 len)
{
    return (HAL_I2C_Mem_Write((I2C_HandleTypeDef*)i2c, dev_addr << 1, reg_addr,
                              I2C_MEMADD_SIZE_8BIT, data, len, 100) == HAL_OK) ? 0 : 1;
}

void bme280_port_delay_ms(u32 ms)
{
    HAL_Delay(ms);
}
