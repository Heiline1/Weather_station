#include "test.h"
#include "mcu_wrapper.h"
#include "model/model.h"
#include "log/logger.h"
#include <string.h>


static void test_for_oled(void);

static void test_for_temt6000(void);
static void test_for_bme280(void);


void test_hw_init(void)
{
    log_init();
    log_info("========== HW Test Start ==========");
    model_init();
    // data_acquisition_init();
    log_info("All modules initialized");
}

void test_hw_loop(void)
{
    // test_for_oled();
    // test_for_temt6000(); 
    test_for_bme280();
}




static void test_for_oled(void)
{
    float lux = 0.0f;
    u8 ret;
    char buff[16];

    log_info("OLED test: displaying TEMT6000 value");
    ssd1306_clear(&g_oled);
    ssd1306_show_string(&g_oled, 0, 0, "TEMT6000 TEST", OLED_FONT_8X16, 0);

    while (1) {
        ret = temt6000_read_lux(&g_temt6000, &lux);
        if (ret == TEMT6000_OK) {
            f32_to_str(buff, lux, 1);
            ssd1306_show_string(&g_oled, 0, 2, "Lux:       ", OLED_FONT_8X16, 0);
            ssd1306_show_string(&g_oled, 48, 2, buff, OLED_FONT_8X16, 0);
            ssd1306_show_string(&g_oled, 0, 4, "RTT log active", OLED_FONT_6X8, 0);
            log_info("TEMT6000: %s lux", buff);
        } else {
            ssd1306_show_string(&g_oled, 0, 2, "ADC Error   ", OLED_FONT_8X16, 0);
            log_error("TEMT6000 read failed, err=%d", ret);
        }
        HAL_Delay(1000);
    }
}



static void test_for_temt6000(void)
{
    float lux = 0.0f;
    u8 ret;
    char buff1[16];

    log_info("TEMT6000 test: reading every second");

    while (1) {
        ret = temt6000_read_lux(&g_temt6000, &lux);
        if (ret == TEMT6000_OK) {
            f32_to_str(buff1, lux, 1);
            log_info("TEMT6000: %s lux", buff1);
        } else {
            log_error("TEMT6000 read failed, err=%d", ret);
        }
        HAL_Delay(1000);
    }
}


static void test_for_bme280(void)
{
    float temp = 0.0f;
    float press = 0.0f;
    float humi = 0.0f;
    u8 ret;
    char tbuf[16];
    char pbuf[16];
    char hbuf[16];

    log_info("BME280 test: reading every second");

    while (1) {
        ret = bme280_read_all(&g_bme280, &temp, &press, &humi);
        if (ret == BME280_OK) {
            f32_to_str(tbuf, temp, 1);
            f32_to_str(pbuf, press, 1);
            f32_to_str(hbuf, humi, 1);
            log_info("BME280 T:%s C  P:%s hPa  H:%s %%", tbuf, pbuf, hbuf);
        } else {
            log_error("BME280 read failed, err=%d", ret);
        }
        HAL_Delay(1000);
    }
}
