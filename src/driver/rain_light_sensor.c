#include "rain_light_sensor.h"
#include <string.h>

// Modbus CRC16 计算函数
static uint16_t rls_crc16(const uint8_t *buf, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for (uint16_t pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)buf[pos];
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc; // 返回低字节在前，高字节在后的标准格式
}

// 发送指令并启动接收
static bool rls_send_cmd(rls_t *self, uint8_t *cmd, uint16_t len) {
    if (self->is_busy) return false;
    
    self->is_busy  = true;
    self->is_ready = false;
    
    // 清空接收缓冲区
    memset(self->rx_buf, 0, self->rx_buf_size);
    
    // 开启DMA接收 (假设上层已配置好DMA)
    HAL_UART_Receive_DMA(self->huart, self->rx_buf, self->rx_buf_size);
    
    // 发送命令 (使用DMA发送)
    if (HAL_UART_Transmit_DMA(self->huart, cmd, len) != HAL_OK) {
        self->is_busy = false;
        return false;
    }
    
    return true;
}

// 等待应答完成 (轮询方式，实际项目中可根据RTOS改为信号量等待)
static bool rls_wait_response(rls_t *self, uint32_t timeout_ms) {
    uint32_t tickstart = HAL_GetTick();
    while (!self->is_ready) {
        if ((HAL_GetTick() - tickstart) > timeout_ms) {
            self->is_busy = false;
            HAL_UART_DMAStop(self->huart); // 超时停止DMA
            return false; // 超时
        }
    }
    self->is_busy = false;
    return true;
}

// ==================== 接口实现 ====================

void rls_init(rls_t *self, UART_HandleTypeDef *huart, uint8_t dev_addr, uint8_t *rx_buf, uint16_t rx_buf_size) {
    self->huart       = huart;
    self->dev_addr    = dev_addr;
    self->rx_buf      = rx_buf;
    self->rx_buf_size = rx_buf_size;
    self->is_busy     = false;
    self->is_ready    = false;
    
    self->data.rainfall_mm     = 0.0f;
    self->data.illuminance_lux = 0;
}

bool rls_read_rainfall(rls_t *self) {
    uint8_t cmd[8];
    cmd[0] = self->dev_addr;
    cmd[1] = 0x03; // 功能码
    cmd[2] = 0x00; cmd[3] = 0x00; // 起始地址
    cmd[4] = 0x00; cmd[5] = 0x01; // 数据长度
    uint16_t crc = rls_crc16(cmd, 6);
    cmd[6] = crc & 0xFF;       // CRC低
    cmd[7] = (crc >> 8) & 0xFF;// CRC高

    if (!rls_send_cmd(self, cmd, 8)) return false;
    if (!rls_wait_response(self, 200)) return false;

    // 校验应答 (期望7字节: 地址, 功能码, 字节数, 数据高, 数据低, CRC低, CRC高)
    if (self->rx_buf[0] == self->dev_addr && self->rx_buf[1] == 0x03 && self->rx_buf[2] == 0x02) {
        uint16_t rain_raw = (self->rx_buf[3] << 8) | self->rx_buf[4];
        self->data.rainfall_mm = (float)rain_raw / 10.0f;
        return true;
    }
    return false;
}

bool rls_read_illuminance(rls_t *self) {
    uint8_t cmd[8];
    cmd[0] = self->dev_addr;
    cmd[1] = 0x03;
    cmd[2] = 0x00; cmd[3] = 0x02; // 起始地址
    cmd[4] = 0x00; cmd[5] = 0x02; // 数据长度 (2个寄存器=4字节)
    uint16_t crc = rls_crc16(cmd, 6);
    cmd[6] = crc & 0xFF;
    cmd[7] = (crc >> 8) & 0xFF;

    if (!rls_send_cmd(self, cmd, 8)) return false;
    if (!rls_wait_response(self, 200)) return false;

    // 校验应答 (期望9字节: 地址, 功能码, 字节数(4), 数据x4, CRC低, CRC高)
    if (self->rx_buf[0] == self->dev_addr && self->rx_buf[1] == 0x03 && self->rx_buf[2] == 0x04) {
        self->data.illuminance_lux = (self->rx_buf[3] << 24) | 
                                     (self->rx_buf[4] << 16) | 
                                     (self->rx_buf[5] << 8)  | 
                                      self->rx_buf[6];
        return true;
    }
    return false;
}

bool rls_read_all(rls_t *self) {
    // 这里分两次读取，也可以根据手册写一个合并读取的指令（如果设备支持连续读取0x0000~0x0003）
    if (rls_read_rainfall(self)) {
        HAL_Delay(50); // 适当延时防止总线冲突
        return rls_read_illuminance(self);
    }
    return false;
}

bool rls_clear_rainfall(rls_t *self) {
    uint8_t cmd[8];
    cmd[0] = self->dev_addr;
    cmd[1] = 0x06; // 功能码
    cmd[2] = 0x00; cmd[3] = 0x00; // 起始地址
    cmd[4] = 0x00; cmd[5] = 0x5A; // 清除命令
    uint16_t crc = rls_crc16(cmd, 6);
    cmd[6] = crc & 0xFF;
    cmd[7] = (crc >> 8) & 0xFF;

    if (!rls_send_cmd(self, cmd, 8)) return false;
    if (!rls_wait_response(self, 200)) return false;

    // 原样应答校验
    if (self->rx_buf[0] == self->dev_addr && self->rx_buf[1] == 0x06) {
        return true;
    }
    return false;
}

bool rls_set_address(rls_t *self, uint8_t new_addr) {
    uint8_t cmd[8];
    cmd[0] = self->dev_addr;
    cmd[1] = 0x06;
    cmd[2] = 0x07; cmd[3] = 0xD0; // 寄存器地址
    cmd[4] = 0x00; cmd[5] = new_addr;
    uint16_t crc = rls_crc16(cmd, 6);
    cmd[6] = crc & 0xFF;
    cmd[7] = (crc >> 8) & 0xFF;

    if (!rls_send_cmd(self, cmd, 8)) return false;
    if (!rls_wait_response(self, 200)) return false;

    if (self->rx_buf[0] == self->dev_addr && self->rx_buf[1] == 0x06 && self->rx_buf[5] == new_addr) {
        self->dev_addr = new_addr; // 更新对象记录的地址
        return true;
    }
    return false;
}

bool rls_set_baudrate(rls_t *self, uint8_t baudrate_code) {
    // baudrate_code: 0=2400, 1=4800, 2=9600
    if (baudrate_code > 2) return false;

    uint8_t cmd[8];
    cmd[0] = self->dev_addr;
    cmd[1] = 0x06;
    cmd[2] = 0x07; cmd[3] = 0xD1; // 寄存器地址
    cmd[4] = 0x00; cmd[5] = baudrate_code;
    uint16_t crc = rls_crc16(cmd, 6);
    cmd[6] = crc & 0xFF;
    cmd[7] = (crc >> 8) & 0xFF;

    if (!rls_send_cmd(self, cmd, 8)) return false;
    if (!rls_wait_response(self, 200)) return false;

    if (self->rx_buf[0] == self->dev_addr && self->rx_buf[1] == 0x06 && self->rx_buf[5] == baudrate_code) {
        return true;
    }
    return false;
}

// 需在stm32f4xx_it.c 的 USARTx_IRQHandler 中调用
void rls_irq_handler(rls_t *self) {
    if (__HAL_UART_GET_FLAG(self->huart, UART_FLAG_IDLE)) {
        __HAL_UART_CLEAR_IDLEFLAG(self->huart);
        
        // 停止DMA接收，获取实际接收长度
        HAL_UART_DMAStop(self->huart);
        
        // 标记数据已就绪
        self->is_ready = true;
    }
}
