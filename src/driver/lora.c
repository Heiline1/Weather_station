#include "lora.h"
#include "mcu_wrapper.h"
#include "../log/logger.h"
#include <string.h>
#include <stdio.h>

/* ==================================================================
 * 内部辅助
 * ================================================================== */

/** 等待 AUX 高电平，最大等待 2s */
static void _wait_aux_high(void)
{
    u32 start = HAL_GetTick();
    while (!lora_port_get_aux()) {
        if (HAL_GetTick() - start > 2000) break;
    }
}

/** 发 AT 指令（自动追加 \r\n），读响应到 buf */
static u8 _lora_cmd(lora_t* dev, const char* cmd,
                     u8* resp_buf, u16* resp_len, u32 timeout)
{
    u16 cmd_len = strlen(cmd);

    /* 发送 */
    lora_port_uart_flush(dev->uart);
    if (lora_port_uart_write_buf(dev->uart, (const u8*)cmd, cmd_len))
        return LORA_ERR_UART;
    /* 追加 \r\n（除非是 +++） */
    if (cmd_len != 3 || cmd[0] != '+' || cmd[1] != '+' || cmd[2] != '+') {
        u8 term[2] = { '\r', '\n' };
        if (lora_port_uart_write_buf(dev->uart, term, 2))
            return LORA_ERR_UART;
    }

    /* 读响应 */
    *resp_len = lora_port_uart_read(dev->uart, resp_buf, resp_len ? *resp_len : 0, timeout);
    return LORA_OK;
}

/** 发指令 + 检查 OK 响应 */
static u8 _lora_cmd_ok(lora_t* dev, const char* cmd)
{
    u8 buf[128];
    u16 len = sizeof(buf);
    u8 ret = _lora_cmd(dev, cmd, buf, &len, 2000);
    if (ret) return ret;

    /* 查找 OK 确认 */
    for (u16 i = 0; i + 1 < len; i++) {
        if ((buf[i] == 'O' || buf[i] == 'o') &&
            (buf[i+1] == 'K' || buf[i+1] == 'k'))
            return LORA_OK;
    }
    /* 查找 ERROR */
    for (u16 i = 0; i + 4 < len; i++) {
        if (buf[i] == 'E' && buf[i+1] == 'R' && buf[i+2] == 'R' &&
            buf[i+3] == 'O' && buf[i+4] == 'R')
            return LORA_ERR_RESP;
    }
    return LORA_ERR_TIMEOUT;
}

/** 进入配置模式（AT 指令模式） */
static u8 _lora_enter_config(lora_t* dev)
{
    /* 切换到一般模式（M0=0, M1=0），串口接收必须打开才能收到 +++ */
    lora_set_mode(dev, LORA_MODE_NORMAL);
    HAL_Delay(50);

    /* 清空 UART 缓冲，去除可能的脏数据 */
    lora_port_uart_flush(dev->uart);

    /* 发送 +++（不加 \r\n），等待 OK */
    u8 buf[64];
    u16 len = sizeof(buf);
    u8 ret = _lora_cmd(dev, "+++", buf, &len, 2000);
    if (ret) return ret;

    for (u16 i = 0; i + 1 < len; i++) {
        if ((buf[i] == 'O' || buf[i] == 'o') &&
            (buf[i+1] == 'K' || buf[i+1] == 'k'))
            return LORA_OK;
    }
    return LORA_ERR_TIMEOUT;
}

/* ==================================================================
 * 基础操作
 * ================================================================== */

u8 lora_init(lora_t* dev)
{
    lora_set_mode(dev, LORA_MODE_NORMAL);
    dev->baud      = 9600;
    dev->parity    = 0;
    dev->airspeed  = 2400;
    dev->power     = 22;
    dev->tx_freq_idx  = 23;
    dev->rx_freq_idx  = 23;
    dev->address   = 0;
    dev->work_mode = LORA_WORK_TRANSPARENT;
    dev->ms_mode   = LORA_MS_MASTER;
    dev->wakeup_ms = 250;
    dev->ldreh     = false;
    dev->chcheck   = true;
    dev->activities = true;
    dev->configured = false;
    return LORA_OK;
}

void lora_set_mode(lora_t* dev, u8 mode)
{
    switch (mode) {
    case LORA_MODE_NORMAL:
        lora_port_set_m0(false);
        lora_port_set_m1(false);
        break;
    case LORA_MODE_WAKEUP:
        lora_port_set_m0(true);
        lora_port_set_m1(false);
        break;
    case LORA_MODE_CONFIG:
        lora_port_set_m0(false);
        lora_port_set_m1(true);
        break;
    case LORA_MODE_SLEEP:
        lora_port_set_m0(true);
        lora_port_set_m1(true);
        break;
    }
    _wait_aux_high();
    dev->cur_mode = mode;
}

u8 lora_get_mode(lora_t* dev) { return dev->cur_mode; }

bool lora_is_busy(lora_t* dev)
{
    (void)dev;
    return !lora_port_get_aux();
}

u8 lora_send_data(lora_t* dev, const u8* data, u16 len)
{
    if (dev->cur_mode != LORA_MODE_NORMAL && dev->cur_mode != LORA_MODE_WAKEUP)
        return LORA_ERR_MODE;

    _wait_aux_high();
    if (lora_port_uart_write_buf(dev->uart, data, len))
        return LORA_ERR_UART;
    return LORA_OK;
}

u8 lora_recv_data(lora_t* dev, u8* buf, u16 max_len, u16* len)
{
    if (dev->cur_mode != LORA_MODE_NORMAL && dev->cur_mode != LORA_MODE_WAKEUP)
        return LORA_ERR_MODE;

    *len = lora_port_uart_read_buf(dev->uart, buf, max_len);
    return LORA_OK;
}

/* ==================================================================
 * 系统指令
 * ================================================================== */

u8 lora_save_config(lora_t* dev)
{
    u8 ret = _lora_enter_config(dev);
    if (ret) return ret;
    ret = _lora_cmd_ok(dev, "AT&W");
    lora_set_mode(dev, LORA_MODE_NORMAL);
    return ret;
}

u8 lora_reboot(lora_t* dev)
{
    u8 ret = _lora_enter_config(dev);
    if (ret) return ret;
    /* AT&R 返回 OK 后立即重启，不返回 Normal 模式 */
    _lora_cmd_ok(dev, "AT&R");
    HAL_Delay(500);
    lora_set_mode(dev, LORA_MODE_NORMAL);
    return LORA_OK;
}

u8 lora_factory_reset(lora_t* dev)
{
    u8 ret = _lora_enter_config(dev);
    if (ret) return ret;
    _lora_cmd_ok(dev, "AT&F");
    HAL_Delay(500);
    lora_set_mode(dev, LORA_MODE_NORMAL);
    dev->configured = false;
    return LORA_OK;
}

/* ==================================================================
 * 查询指令
 * ================================================================== */

u8 lora_get_version(lora_t* dev, char* buf, u16 max_len)
{
    u8 ret = _lora_enter_config(dev);
    if (ret) return ret;

    u16 len = max_len;
    ret = _lora_cmd(dev, "AT+CGMR", (u8*)buf, &len, 3000);
    lora_set_mode(dev, LORA_MODE_NORMAL);
    if (ret == LORA_OK && len < max_len)
        buf[len] = '\0';
    return ret;
}

u8 lora_get_all_params(lora_t* dev, char* buf, u16 max_len)
{
    u8 ret = _lora_enter_config(dev);
    if (ret) return ret;

    u16 len = max_len;
    ret = _lora_cmd(dev, "AT+DTUALL?", (u8*)buf, &len, 3000);
    lora_set_mode(dev, LORA_MODE_NORMAL);
    if (ret == LORA_OK && len < max_len)
        buf[len] = '\0';
    return ret;
}

u8 lora_get_sn(lora_t* dev, char* buf, u16 max_len)
{
    u8 ret = _lora_enter_config(dev);
    if (ret) return ret;

    u16 len = max_len;
    ret = _lora_cmd(dev, "AT+SNSTR?", (u8*)buf, &len, 3000);
    lora_set_mode(dev, LORA_MODE_NORMAL);
    if (ret == LORA_OK && len < max_len)
        buf[len] = '\0';
    return ret;
}

/* ==================================================================
 * 基本参数配置（内部辅助宏）
 * ================================================================== */

/**
 * @brief 通用参数配置：进入配置模式 → 发 AT 指令 → 退出配置模式
 */
static u8 _lora_set_param(lora_t* dev, const char* full_cmd)
{
    u8 ret = _lora_enter_config(dev);
    if (ret) return ret;
    ret = _lora_cmd_ok(dev, full_cmd);
    lora_set_mode(dev, LORA_MODE_NORMAL);
    return ret;
}

/* ---------- 串口参数 ---------- */

/**
 * @brief 配置串口波特率
 * @note 模块收到该指令后立即切换波特率。调用本函数后 MCU 的 UART
 *       仍为旧波特率，无法继续与模块通信。正确流程：
 *       lora_config_uart() → lora_save_config() → lora_reboot()
 *       → 重新配置 MCU UART 为新波特率 → lora_init()
 *
 *       本函数会返回 LORA_OK（指令已发出），但后续的退出配置模式
 *       可能失败（被忽略），属于正常行为。
 */
u8 lora_config_uart(lora_t* dev, u32 baud, u8 parity)
{
    char cmd[32];
    u16 n = (u16)sprintf(cmd, "AT+UART=%lu,%u",
                          (unsigned long)baud, (unsigned)parity);
    if (n >= sizeof(cmd)) return LORA_ERR_UART;

    u8 ret = _lora_enter_config(dev);
    if (ret) return ret;

    /* 发送指令（不等待 OK — 模块可能已切换波特率） */
    lora_port_uart_flush(dev->uart);
    if (lora_port_uart_write_buf(dev->uart, (const u8*)cmd, n)) {
        lora_set_mode(dev, LORA_MODE_NORMAL);
        return LORA_ERR_UART;
    }
    u8 term[2] = { '\r', '\n' };
    lora_port_uart_write_buf(dev->uart, term, 2);

    /* 不等待响应，直接退出配置模式到 NORMAL（波特率可能已变） */
    lora_set_mode(dev, LORA_MODE_NORMAL);

    dev->baud   = baud;
    dev->parity = parity;
    return LORA_OK;
}

/* ---------- 空中速率 ---------- */

u8 lora_config_airspeed(lora_t* dev, u32 speed)
{
    char cmd[24];
    u16 n = (u16)sprintf(cmd, "AT+AIRSPEED=%lu", (unsigned long)speed);
    if (n >= sizeof(cmd)) return LORA_ERR_UART;
    dev->airspeed = speed;
    return _lora_set_param(dev, cmd);
}

/* ---------- 发射功率 ---------- */

u8 lora_config_power(lora_t* dev, u8 power)
{
    char cmd[16];
    u16 n = (u16)sprintf(cmd, "AT+POWER=%u", (unsigned)power);
    if (n >= sizeof(cmd)) return LORA_ERR_UART;
    dev->power = power;
    return _lora_set_param(dev, cmd);
}

/* ---------- 发射频率 ---------- */

u8 lora_config_tx_freq(lora_t* dev, u8 ch)
{
    char cmd[16];
    u16 n = (u16)sprintf(cmd, "AT+TXFREQ=%u", (unsigned)ch);
    if (n >= sizeof(cmd)) return LORA_ERR_UART;
    dev->tx_freq_idx = ch;
    return _lora_set_param(dev, cmd);
}

/* ---------- 接收频率 ---------- */

u8 lora_config_rx_freq(lora_t* dev, u8 ch)
{
    char cmd[16];
    u16 n = (u16)sprintf(cmd, "AT+RXFREQ=%u", (unsigned)ch);
    if (n >= sizeof(cmd)) return LORA_ERR_UART;
    dev->rx_freq_idx = ch;
    return _lora_set_param(dev, cmd);
}

/* ---------- 收发频率一并设置 ---------- */

u8 lora_config_rf_freq(lora_t* dev, u8 tx_ch, u8 rx_ch)
{
    u8 ret = lora_config_tx_freq(dev, tx_ch);
    if (ret) return ret;
    return lora_config_rx_freq(dev, rx_ch);
}

/* ---------- 地址 ---------- */

u8 lora_config_address(lora_t* dev, u16 addr)
{
    char cmd[20];
    u16 n = (u16)sprintf(cmd, "AT+ADDRESS=%u", (unsigned)addr);
    if (n >= sizeof(cmd)) return LORA_ERR_UART;
    dev->address = addr;
    return _lora_set_param(dev, cmd);
}

/* ---------- 工作模式 ---------- */

u8 lora_config_workmode(lora_t* dev, u8 mode)
{
    char cmd[16];
    u16 n = (u16)sprintf(cmd, "AT+WORKMODE=%u", (unsigned)mode);
    if (n >= sizeof(cmd)) return LORA_ERR_UART;
    dev->work_mode = mode;
    return _lora_set_param(dev, cmd);
}

/* ---------- 唤醒间隔 ---------- */

u8 lora_config_wakeup(lora_t* dev, u16 ms)
{
    char cmd[16];
    u16 n = (u16)sprintf(cmd, "AT+WAKEUP=%u", (unsigned)ms);
    if (n >= sizeof(cmd)) return LORA_ERR_UART;
    dev->wakeup_ms = ms;
    return _lora_set_param(dev, cmd);
}

/* ---------- 速率优化 ---------- */

u8 lora_config_ldreh(lora_t* dev, bool enable)
{
    u8 ret = _lora_set_param(dev, enable ? "AT+LDREH=1" : "AT+LDREH=0");
    if (ret == LORA_OK) dev->ldreh = enable;
    return ret;
}

/* ---------- 信道接收过滤 ---------- */

u8 lora_config_chcheck(lora_t* dev, bool enable)
{
    u8 ret = _lora_set_param(dev, enable ? "AT+CHCHECK=1" : "AT+CHCHECK=0");
    if (ret == LORA_OK) dev->chcheck = enable;
    return ret;
}

/* ---------- 信道避让 ---------- */

u8 lora_config_activities(lora_t* dev, bool enable)
{
    u8 ret = _lora_set_param(dev, enable ? "AT+ACTIVITIES=1" : "AT+ACTIVITIES=0");
    if (ret == LORA_OK) dev->activities = enable;
    return ret;
}

/* ---------- 主从模式 ---------- */

u8 lora_config_msmode(lora_t* dev, u8 role)
{
    char cmd[16];
    u16 n = (u16)sprintf(cmd, "AT+MSMODE=%u", (unsigned)role);
    if (n >= sizeof(cmd)) return LORA_ERR_UART;
    dev->ms_mode = role;
    return _lora_set_param(dev, cmd);
}

/* ---------- 中继模式 ---------- */

u8 lora_config_relay_mode(lora_t* dev, u8 mode)
{
    char cmd[16];
    u16 n = (u16)sprintf(cmd, "AT+RELAYMODE=%u", (unsigned)mode);
    if (n >= sizeof(cmd)) return LORA_ERR_UART;
    return _lora_set_param(dev, cmd);
}

/* ---------- 中继网络 ID ---------- */

u8 lora_config_netid(lora_t* dev, u8 netid)
{
    char cmd[16];
    u16 n = (u16)sprintf(cmd, "AT+NETID=%u", (unsigned)netid);
    if (n >= sizeof(cmd)) return LORA_ERR_UART;
    return _lora_set_param(dev, cmd);
}

/* ==================================================================
 * 轮询配置
 * ================================================================== */

u8 lora_config_poll(lora_t* dev, bool enable, u16 interval_ms, u8 fmt)
{
    char cmd[32];
    u16 n = (u16)sprintf(cmd, "AT+POLL=%u,%u,%u",
                          (unsigned)(enable ? 1 : 0),
                          (unsigned)interval_ms,
                          (unsigned)fmt);
    if (n >= sizeof(cmd)) return LORA_ERR_UART;
    return _lora_set_param(dev, cmd);
}

u8 lora_config_poll_str(lora_t* dev, u8 idx, bool enable,
                         bool crc, const char* hex_data)
{
    char cmd[64];
    u16 n = (u16)sprintf(cmd, "AT+POLLSTR=%u,%u,%u,\"%s\"",
                          (unsigned)idx,
                          (unsigned)(enable ? 1 : 0),
                          (unsigned)(crc ? 1 : 0),
                          hex_data ? hex_data : "");
    if (n >= sizeof(cmd)) return LORA_ERR_UART;
    return _lora_set_param(dev, cmd);
}

/* ==================================================================
 * 工具
 * ================================================================== */

u8 lora_wait_ready(lora_t* dev, u32 timeout_ms)
{
    (void)dev;
    u32 start = HAL_GetTick();
    while (!lora_port_get_aux()) {
        if (HAL_GetTick() - start > timeout_ms)
            return LORA_ERR_TIMEOUT;
    }
    return LORA_OK;
}

void lora_sync_params(lora_t* dev)
{
    char buf[256];
    u16 len = sizeof(buf);
    u8 ret = _lora_enter_config(dev);
    if (ret) goto done;

    ret = _lora_cmd(dev, "AT+DTUALL?", (u8*)buf, &len, 3000);
    if (ret) goto done;

    /* 从 DTUALL 输出中解析常用参数 */
    /* 格式示例: +UART:9600,0  +ADDRESS:0  +WORKMODE:0  +AIRSPEED:2400 ... */
    buf[len < sizeof(buf) ? len : sizeof(buf) - 1] = '\0';

    char* line = buf;
    while (line && *line) {
        /* 找到行尾 */
        char* nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        int v;
        if      (sscanf(line, "+UART:%u,%hhu", &dev->baud, &dev->parity) >= 1) { /* ok */ }
        else if (sscanf(line, "+ADDRESS:%hu", &dev->address) >= 1)             { /* ok */ }
        else if (sscanf(line, "+WORKMODE:%hhu", &dev->work_mode) >= 1)         { /* ok */ }
        else if (sscanf(line, "+AIRSPEED:%u", &dev->airspeed) >= 1)            { /* ok */ }
        else if (sscanf(line, "+POWER:%hhu", &dev->power) >= 1)                { /* ok */ }
        else if (sscanf(line, "+TXFREQ:%hhu", &dev->tx_freq_idx) >= 1)         { /* ok */ }
        else if (sscanf(line, "+RXFREQ:%hhu", &dev->rx_freq_idx) >= 1)         { /* ok */ }
        else if (sscanf(line, "+WAKEUP:%u", &v) >= 1)                          { dev->wakeup_ms = (u16)v; }
        else if (sscanf(line, "+CHCHECK:%hhu", &v) >= 1)                       { dev->chcheck = (v != 0); }
        else if (sscanf(line, "+ACTIVITIES:%hhu", &v) >= 1)                    { dev->activities = (v != 0); }
        else if (sscanf(line, "+MSMODE:%hhu", &dev->ms_mode) >= 1)             { /* ok */ }
        else if (sscanf(line, "+LowDatarateEn:%hhu", &v) >= 1)                 { dev->ldreh = (v != 0); }

        line = nl ? nl + 1 : NULL;
    }

    dev->configured = true;

done:
    lora_set_mode(dev, LORA_MODE_NORMAL);
}
