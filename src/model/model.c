#include "model.h"
#include "mcu_wrapper.h"
#include "log/rtt_logger.h"

#include "driver/bme280.h"

bme280_t g_bme280 = {

    .i2c = &hi2c2,
    .addr = BME280_I2C_ADDR_0,

};

ssd1306_t g_oled = {

    .i2c = &hi2c1,

};

temt6000_t g_temt6000 = {

    .adc = &hadc1,
    .rank = ADC_REGULAR_RANK_1,

};

lora_t g_lora = {

    .uart = &huart2,
    .cur_mode = LORA_MODE_NORMAL,

};

uint8_t sensor_rx_buf[32]; // 接收缓冲区
rls_t g_rls = {
    .huart = &huart1;
};

void model_init(void)
{
    ssd1306_init(&g_oled);
    u8 ret;

    lora_init(&g_lora);
    rlog_push_tag("Lora_Ready");

    ret = bme280_init(&g_bme280);
    if (ret) {
        log_error("BME280 init failed, err=%d", ret);
    }


    if (temt6000_init(&g_temt6000)) {
        log_error("TEMT6000 init failed");
    }

    /* 清除 GDDRAM 随机值，防止显示屏上电花屏 */
    ssd1306_clear(&g_oled);

    rls_init(&my_sensor, &huart1, RLS_DEFAULT_ADDR, sensor_rx_buf, sizeof(sensor_rx_buf));
}