#ifndef DRIVER_LORA_H
#define DRIVER_LORA_H

#include "../util/datatype.h"

/* ========== Port 层声明 ========== */

extern void lora_port_delay_ms(u32 ms);
extern u8   lora_port_uart_write_buf(void* uart, const u8* data, u16 len);
extern u16  lora_port_uart_read_buf(void* uart, u8* buf, u16 max_len);
extern u16  lora_port_uart_read(void* uart, u8* buf, u16 max_len, u32 timeout);
extern void lora_port_uart_flush(void* uart);
extern void lora_port_set_m0(bool level);
extern void lora_port_set_m1(bool level);
extern bool lora_port_get_aux(void);

/* ========== 硬件模式 ========== */

#define LORA_MODE_NORMAL    0
#define LORA_MODE_WAKEUP    1
#define LORA_MODE_CONFIG    2
#define LORA_MODE_SLEEP     3

/* ========== 返回码 ========== */

#define LORA_OK              0
#define LORA_ERR_MODE        1   /* 模式错误（不在 CONFIG 模式） */
#define LORA_ERR_UART        2   /* UART 发送失败 */
#define LORA_ERR_TIMEOUT     3   /* 模块响应超时 */
#define LORA_ERR_RESP        4   /* 模块返回 ERROR */

/* ========== 工作模式 (AT+WORKMODE) ========== */
#define LORA_WORK_TRANSPARENT   0   /* 透传 */
#define LORA_WORK_FIXED         1   /* 定点 */
#define LORA_WORK_MASTER_SLAVE  2   /* 主从 */
#define LORA_WORK_MESH          3   /* 自组网 */

/* ========== 主从模式 (AT+MSMODE) ========== */
#define LORA_MS_MASTER  0   /* 主机 */
#define LORA_MS_SLAVE   1   /* 从机 */

/* ========== 中继模式 (AT+RELAYMODE) ========== */
#define LORA_RELAY_NODE    0   /* 节点 */
#define LORA_RELAY_BRIDGE  1   /* 中继 */

/* ========== 设备结构体 ========== */

typedef struct {
    void* uart;             /* UART 句柄指针 */
    u8    cur_mode;         /* 当前 M0/M1 硬件模式 */

    /* 缓存的最新配置参数（用于快速查询，不一定与模块实时同步） */
    u32   baud;             /* 串口波特率 */
    u8    parity;           /* 校验位：0=无 1=奇 2=偶 */
    u32   airspeed;         /* 空中速率 (bps) */
    u8    power;            /* 发射功率 (dBm, 10-22) */
    u8    tx_freq_idx;      /* 发射频率索引 (0-100 → 410-510MHz) */
    u8    rx_freq_idx;      /* 接收频率索引 */
    u16   address;          /* 定点/主从地址 (0-65535) */
    u8    work_mode;        /* 数据传输模式 */
    u8    ms_mode;          /* 主从角色 */
    u16   wakeup_ms;        /* 唤醒间隔 (ms) */
    bool  ldreh;            /* 速率优化 */
    bool  chcheck;          /* 信道接收过滤 */
    bool  activities;       /* 信道避让 */
    bool  configured;       /* 配置是否已从模块读取过 */
} lora_t;

/* ========== 基础操作 ========== */

u8   lora_init(lora_t* dev);
void lora_set_mode(lora_t* dev, u8 mode);
u8   lora_get_mode(lora_t* dev);
bool lora_is_busy(lora_t* dev);
u8   lora_send_data(lora_t* dev, const u8* data, u16 len);
u8   lora_recv_data(lora_t* dev, u8* buf, u16 max_len, u16* len);

/* ========== 系统指令 ========== */

u8   lora_save_config(lora_t* dev);        /* AT&W */
u8   lora_reboot(lora_t* dev);             /* AT&R */
u8   lora_factory_reset(lora_t* dev);       /* AT&F */

/* ========== 查询指令 ========== */

u8   lora_get_version(lora_t* dev, char* buf, u16 max_len);    /* AT+CGMR */
u8   lora_get_all_params(lora_t* dev, char* buf, u16 max_len); /* AT+DTUALL? */
u8   lora_get_sn(lora_t* dev, char* buf, u16 max_len);         /* AT+SNSTR? */

/* ========== 基本参数配置 ========== */

u8   lora_config_uart(lora_t* dev, u32 baud, u8 parity);          /* AT+UART */
u8   lora_config_airspeed(lora_t* dev, u32 speed);                 /* AT+AIRSPEED */
u8   lora_config_power(lora_t* dev, u8 power);                     /* AT+POWER */
u8   lora_config_tx_freq(lora_t* dev, u8 ch);                     /* AT+TXFREQ */
u8   lora_config_rx_freq(lora_t* dev, u8 ch);                     /* AT+RXFREQ */
u8   lora_config_rf_freq(lora_t* dev, u8 tx_ch, u8 rx_ch);        /* TX+RX 一起设 */
u8   lora_config_address(lora_t* dev, u16 addr);                   /* AT+ADDRESS */
u8   lora_config_workmode(lora_t* dev, u8 mode);                   /* AT+WORKMODE */
u8   lora_config_wakeup(lora_t* dev, u16 ms);                      /* AT+WAKEUP */
u8   lora_config_ldreh(lora_t* dev, bool enable);                  /* AT+LDREH */
u8   lora_config_chcheck(lora_t* dev, bool enable);                /* AT+CHCHECK */
u8   lora_config_activities(lora_t* dev, bool enable);             /* AT+ACTIVITIES */
u8   lora_config_msmode(lora_t* dev, u8 role);                     /* AT+MSMODE */
u8   lora_config_relay_mode(lora_t* dev, u8 mode);                 /* AT+RELAYMODE */
u8   lora_config_netid(lora_t* dev, u8 netid);                     /* AT+NETID */

/* ========== 轮询配置 ========== */

u8   lora_config_poll(lora_t* dev, bool enable, u16 interval_ms, u8 fmt);    /* AT+POLL */
u8   lora_config_poll_str(lora_t* dev, u8 idx, bool enable,
                          bool crc, const char* hex_data);                    /* AT+POLLSTR */

/* ========== 工具 ========== */

u8   lora_wait_ready(lora_t* dev, u32 timeout_ms);   /* 等待 AUX 高电平 */
void lora_sync_params(lora_t* dev);                   /* 调用 AT+DTUALL? 刷新缓存的参数 */

#endif /* DRIVER_LORA_H */
