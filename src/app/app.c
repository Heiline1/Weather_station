#include "app.h"

#include "model/model.h"
#include "log/logger.h"
#include "mcu_wrapper.h"
#include "log/rtt_logger.h"

void app_init(void)
{
    
    log_init();
    log_info("program start");
    model_init();


    log_info("system init complete");
}

void app_run(void)
{
    static u32 last_acq = 0;

    
    while (1) {
        log_info("app running");
        if (rls_read_all(&my_sensor)) {
            // 读取成功，使用数据
            float rain = my_sensor.data.rainfall_mm;
            uint32_t lux = my_sensor.data.illuminance_lux;
            
            printf("Rain: %.1f mm, Lux: %lu\r\n", rain, lux);
        } else {
            printf("Sensor read failed!\r\n");
        }
        
        HAL_Delay(2000); // 每2秒读取一次
    }
}
