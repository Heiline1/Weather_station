-- 规则：STM32F1xx HAL 驱动
rule("stm32.f1xx.hal")
    on_load(function (target)
        local src_dir = path.join(os.scriptdir(), "Drivers/STM32F1xx_HAL_Driver/Src")
        local inc_dir = path.join(os.scriptdir(), "Drivers/STM32F1xx_HAL_Driver/Inc")
        target:add("files", path.join(src_dir, "*.c"))
        target:remove("files", path.join(src_dir, "*_template.c"))
        target:remove("files", path.join(src_dir, "stm32f1xx_hal_flash_ramfunc.c"))
        target:add("includedirs", {inc_dir, path.join(inc_dir, "Legacy")})
        target:add("defines", "USE_HAL_DRIVER")
    end)
rule_end()

-- 规则：STM32F1xx CMSIS 头文件路径
rule("stm32.f1xx.cmsis")
    on_load(function (target)
        local base_dir = path.join(os.scriptdir(), "Drivers/CMSIS/Device/ST/STM32F1xx")
        local device_inc_dir = path.join(base_dir, "Include")
        local cmsis_inc_dir = path.join(os.scriptdir(), "Drivers/CMSIS/Include")
        target:add("includedirs", {device_inc_dir, cmsis_inc_dir})
    end)
rule_end()

-- 规则：FreeRTOS（当前未使用）
rule("stm32.f1xx.hal.freertos")
    on_load(function (target)
        local root_dir = os.scriptdir()
        target:add("files", {
            path.join(root_dir, "Middlewares/Third_Party/FreeRTOS/Source/*.c"),
            path.join(root_dir, "Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/*.c"),
            path.join(root_dir, "Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c"),
            path.join(root_dir, "Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3/*.c")
        })
        target:add("includedirs", {
            path.join(root_dir, "Middlewares/Third_Party/FreeRTOS/Source/include"),
            path.join(root_dir, "Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2"),
            path.join(root_dir, "Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3")
        })
    end)
rule_end()
