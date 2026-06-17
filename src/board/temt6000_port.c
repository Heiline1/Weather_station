#include "mcu_wrapper.h"
#include "../util/datatype.h"

u8 temt6000_port_adc_start_injected(void* adc)
{
    return (HAL_ADCEx_InjectedStart((ADC_HandleTypeDef*)adc) == HAL_OK) ? 0 : 1;
}

u8 temt6000_port_adc_poll_injected(void* adc, u32 timeout)
{
    return (HAL_ADCEx_InjectedPollForConversion((ADC_HandleTypeDef*)adc, timeout) == HAL_OK) ? 0 : 1;
}

u32 temt6000_port_adc_get_injected(void* adc, u32 rank)
{
    return HAL_ADCEx_InjectedGetValue((ADC_HandleTypeDef*)adc, rank);
}
