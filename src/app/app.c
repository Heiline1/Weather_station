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
        HAL_Delay(1000);
    }
}
