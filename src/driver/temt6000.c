#include "temt6000.h"

u8 temt6000_init(temt6000_t* dev)
{
    if (!dev) return TEMT6000_ERR_PARAM;

    dev->inited = 1;
    return TEMT6000_OK;
}

u8 temt6000_read_lux(temt6000_t* dev, float* lux)
{
    if (!dev || !lux) return TEMT6000_ERR_PARAM;
    if (!dev->inited) return TEMT6000_ERR_INIT;

    if (temt6000_port_adc_start_injected(dev->adc))
        return TEMT6000_ERR_ADC;

    if (temt6000_port_adc_poll_injected(dev->adc, 100))
        return TEMT6000_ERR_ADC;

    u32 raw = temt6000_port_adc_get_injected(dev->adc, dev->rank);

    float voltage_mv = (float)raw * (float)TEMT6000_VREF_MV
                       / (float)TEMT6000_ADC_RESOLUTION;

    *lux = voltage_mv / TEMT6000_SENSITIVITY_MV_PER_LUX;

    return TEMT6000_OK;
}

void temt6000_deinit(temt6000_t* dev)
{
    if (dev) dev->inited = 0;
}
