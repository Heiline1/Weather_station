#ifndef RAIN_LIGHT_SENSOR_H
#define RAIN_LIGHT_SENSOR_H

#include "mcu_wrapper.h"
#include <stdbool.h>
#include <stdint.h>

// 默认设备地址
#define RLS_DEFAULT_ADDR        0x01

// 传感器数据结构体
typedef struct {
    float    rainfall_mm;     // 当前总雨量值
    uint32_t illuminance_lux; // 当前光照度
} rls_data_t;

// 传感器对象结构体
typedef struct {
    UART_HandleTypeDef *huart;         // 使用的串口实例
    uint8_t             dev_addr;      // 设备地址
    uint8_t            *rx_buf;        // 接收缓冲区指针
    uint16_t            rx_buf_size;   // 接收缓冲区大小
    volatile bool       is_busy;       // 是否正在等待应答
    volatile bool       is_ready;      // 数据是否接收完毕
    rls_data_t          data;          // 解析后的数据
} rls_t;

// ==================== 接口函数 (OOP风格) ====================

// 构造函数：初始化对象
void rls_init(rls_t *self, UART_HandleTypeDef *huart, uint8_t dev_addr, uint8_t *rx_buf, uint16_t rx_buf_size);

// 读雨量
bool rls_read_rainfall(rls_t *self);

// 读光照
bool rls_read_illuminance(rls_t *self);

// 读雨量和光照 (合并读取)
bool rls_read_all(rls_t *self);

// 清除雨量
bool rls_clear_rainfall(rls_t *self);

// 修改设备地址
bool rls_set_address(rls_t *self, uint8_t new_addr);

// 修改波特率
bool rls_set_baudrate(rls_t *self, uint8_t baudrate_code);

// 串口空闲中断回调函数 (需在USARTx_IRQHandler中调用)
void rls_irq_handler(rls_t *self);

#endif // RAIN_LIGHT_SENSOR_H
