#include "mcu_wrapper.h"
#include "../util/datatype.h"

void ssd1306_port_delay_ms(u32 ms)
{
   HAL_Delay(ms);
}

void ssd1306_port_i2c_mem_write(void* i2c, u16 dev_addr, u8 mem_addr, u16 mem_addr_size,
                                       u8* data, u16 data_size, u32 timeout)
{
    HAL_I2C_Mem_Write((I2C_HandleTypeDef*)i2c, dev_addr, mem_addr, mem_addr_size, data, data_size,
                  timeout);
}
