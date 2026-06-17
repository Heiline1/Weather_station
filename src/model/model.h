/**
 * @file    model.h
 * @brief   板级设备实例声明
 *
 * 定义本工程使用的全局设备句柄，所有外设对象在此统一管理。
 *
 * 使用方式：
 *   @code
 *   #include "model.h"
 *   model_init();
 *   ssd1306_show_string(&g_oled, 0, 0, "Hello", 16, 0);
 *   @endcode
 */

#ifndef __MODEL_H
#define __MODEL_H

#include "driver/ssd1306.h"
#include "driver/temt6000.h"
#include "driver/lora.h"
#include "driver/bme280.h"
#include "driver/rain_light_sensor.h"

extern ssd1306_t g_oled;
extern temt6000_t g_temt6000;
extern lora_t g_lora;
extern bme280_t g_bme280;
extern rls_t g_rls;

/**
 * @brief 初始化所有板载外设
 *
 * 按依赖顺序依次初始化：
 *   1. OLED   — ssd1306_init()
 *   2. LoRa   — lora_init()
 *   3. BME280 — bme280_init()
 *   4. AHT20  — aht20_init()
 */
void model_init(void);

#endif /* __MODEL_H */