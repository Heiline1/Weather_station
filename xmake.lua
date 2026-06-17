set_config("plat", "cross")
set_config("arch", "arm")

set_project("Weather_station")
set_version("1.0.0")
set_xmakever("3.0.0")

set_languages("c11", "cxx17")

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "."})

includes("arm-none-eabi-custom.lua")
includes("stm32f1xx-rules.lua")

target("Weather_station")
    set_kind("binary")
    set_toolchains("arm-none-eabi-custom")
    set_filename("Weather_station.elf")

    add_rules("util.convert_bin_hex")
    add_rules("stm32.f1xx.cmsis")
    add_rules("stm32.f1xx.hal")

    add_defines("STM32F103xE")
    add_defines("USE_HAL_DRIVER")

    add_files("startup_stm32f103xe.s")
    add_files("Core/Src/*.c")
    add_files("src/**.c")

    add_includedirs("Core/Inc")
    add_includedirs("src")

    add_cflags(
        "-mcpu=cortex-m3",
        "-mthumb",
        "-std=c11",
        "-Wall",
        "-fdata-sections",
        "-ffunction-sections",
        "-g",
        "-gdwarf-2",
        {force = true}
    )
    add_cxxflags(
        "-mcpu=cortex-m3",
        "-mthumb",
        "-std=c++17",
        "-Wall",
        "-fdata-sections",
        "-ffunction-sections",
        "-g",
        "-gdwarf-2",
        {force = true}
    )
    add_asflags(
        "-mcpu=cortex-m3",
        "-mthumb",
        {force = true}
    )
    add_ldflags(
        "-mcpu=cortex-m3",
        "-mthumb",
        "-Wl,--gc-sections",
        "-Wl,-Map=build/output.map",
        "-TSTM32F103XX_FLASH.ld",
        "-lc", "-lm", "-lnosys",
        "--specs=nano.specs",
        "-u _printf_float",
        "-u _scanf_float",
        {linker = true, force = true}
    )
    add_ldflags(
        "-lstdc++",
        "-lsupc++",
        {linker = true, force = true}
    )

    if is_mode("debug") then
        add_defines("DEBUG")
        set_optimize("none")
        set_symbols("debug")
    else
        set_optimize("fast")
    end

    after_load(function (target)
        print("================================")
        print("Source files:")
        print("================================")
        local source_files = target:sourcefiles()
        for _, file in ipairs(source_files) do
            print("  - " .. file)
        end
        print("\n================================")
        print("Include paths:")
        print("================================")
        local include_dirs = target:get("includedirs")
        if include_dirs then
            for _, dir in ipairs(include_dirs) do
                print("  - " .. dir)
            end
        end
    end)
