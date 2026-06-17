/**
 * @file    lora_port.c
 * @brief   LoRa port 层实现（板级适配 — USART2 + GPIO）
 *
 * 硬件资源：
 *   - USART2 (PD5=TX, PD6=RX)
 *   - M0=PG15, M1=PG10, AUX=PD3
 *
 * 由 CubeMX 完成 USART/GPIO 初始化，model 层提供 UART 句柄。
 */

#include "mcu_wrapper.h"
#include "../util/datatype.h"
#include "../driver/lora.h"

#include "gpio.h"

void lora_port_delay_ms(u32 ms)
{
    HAL_Delay(ms);
}

u8 lora_port_uart_write_buf(void* uart, const u8* data, u16 len)
{
    return (HAL_UART_Transmit((UART_HandleTypeDef*)uart, (u8*)data, len, 100) == HAL_OK) ? 0 : 1;
}

u16 lora_port_uart_read_buf(void* uart, u8* buf, u16 max_len)
{
    if (HAL_UART_Receive((UART_HandleTypeDef*)uart, buf, 1, 10) == HAL_OK)
        return 1;
    return 0;
}

/**
 * @brief 带超时的批量读取 — 用于接收 AT 响应字符串
 * @param uart     UART 句柄
 * @param buf      输出缓冲区
 * @param max_len  最大读取字节数
 * @param timeout  总超时时间 (ms)
 * @return 实际读到的字节数
 */
u16 lora_port_uart_read(void* uart, u8* buf, u16 max_len, u32 timeout)
{
    u16 count = 0;
    u32 start = HAL_GetTick();

    while (count < max_len) {
        u8 byte;
        if (HAL_UART_Receive((UART_HandleTypeDef*)uart, &byte, 1, 50) == HAL_OK) {
            buf[count++] = byte;
            start = HAL_GetTick();  // 收到字节重置超时
        } else {
            if (HAL_GetTick() - start > timeout)
                break;              // 总超时到期
            if (count > 0)
                break;              // 已收到数据且当前无新字节 → 响应结束
        }
    }
    return count;
}

/**
 * @brief 清空 UART RX 缓冲 — AT 交互前清除陈旧数据
 */
void lora_port_uart_flush(void* uart)
{
    u8 dummy;
    UART_HandleTypeDef* huart = (UART_HandleTypeDef*)uart;
    while (HAL_UART_Receive(huart, &dummy, 1, 10) == HAL_OK)
        ;
}

void lora_port_set_m0(bool level)
{
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_15, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void lora_port_set_m1(bool level)
{
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

bool lora_port_get_aux(void)
{
    return HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_3) == GPIO_PIN_SET;
}
