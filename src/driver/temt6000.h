#ifndef DRIVER_TEMT6000_H
#define DRIVER_TEMT6000_H

#include "../util/datatype.h"

/* ===== port 层函数（由 board/temt6000_port.c 实现） ===== */

extern u8   temt6000_port_adc_start_injected(void* adc);
extern u8   temt6000_port_adc_poll_injected(void* adc, u32 timeout);
extern u32  temt6000_port_adc_get_injected(void* adc, u32 rank);

/* ===== 默认配置 ===== */

#define TEMT6000_VREF_MV       3300
#define TEMT6000_ADC_RESOLUTION  4096
#define TEMT6000_SENSITIVITY_MV_PER_LUX  0.18f

/* ===== 返回码 ===== */

#define TEMT6000_OK         0
#define TEMT6000_ERR_ADC    1
#define TEMT6000_ERR_PARAM  2
#define TEMT6000_ERR_INIT   3

/* ===== 设备句柄 ===== */

typedef struct {
    void* adc;
    u32   rank;
    u8    inited;
} temt6000_t;

/* ===== 驱动 API ===== */

u8  temt6000_init(temt6000_t* dev);
u8  temt6000_read_lux(temt6000_t* dev, float* lux);
void temt6000_deinit(temt6000_t* dev);

#endif /* DRIVER_TEMT6000_H */
