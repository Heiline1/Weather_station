toolchain("arm-none-eabi-custom")
    set_kind("standalone")

    local function check_gcc_in(bin_dir)
        local ext = is_host("windows") and ".exe" or ""
        local gcc_path = path.join(bin_dir, "arm-none-eabi-gcc" .. ext)
        return os.isfile(gcc_path)
    end

    on_load(function (toolchain)
        import("lib.detect.find_tool")

        local sdk_root = nil
        local bin_dir = nil

        local config_sdk = toolchain:config("sdk")

        if config_sdk then
            sdk_root = config_sdk
            bin_dir = path.join(sdk_root, "bin")
        end

        if not sdk_root then
            local env_sdk = os.getenv("ARM_GCC_SDK")
            if env_sdk and os.isdir(env_sdk) then
                sdk_root = env_sdk
                bin_dir = path.join(sdk_root, "bin")
                print(">>> Using ARM_GCC_SDK from environment: " .. sdk_root)
            end
        end

        if not sdk_root then
            local tool = find_tool("arm-none-eabi-gcc")
            if tool then
                bin_dir = path.directory(tool.program)
                sdk_root = path.directory(bin_dir)
                print(">>> Auto-detected ARM Toolchain SDK Root: " .. sdk_root)
            end
        end

        if sdk_root and bin_dir and check_gcc_in(bin_dir) then
            local function trim(s)
                if not s then return s end
                return s:match("^%s*(.-)%s*$")
            end
            sdk_root = trim(sdk_root)
            bin_dir = trim(bin_dir)

            if bin_dir:lower():endswith("bin") then
                sdk_root = path.directory(bin_dir)
            end

            toolchain:set("sdkdir", sdk_root)
            toolchain:set("bindir", bin_dir)

            local incdir = path.join(sdk_root, "arm-none-eabi/include")
            print(">>> Adding standard library include path: " .. incdir)
            toolchain:add("includedirs", incdir)

            local function has(prog)
                return os.isfile(path.join(bin_dir, prog .. (is_host("windows") and ".exe" or "")))
            end
            print(" >>> ARM Toolchain SDK Root: " .. sdk_root)
            print(" >>> ARM Toolchain Bin Dir: " .. bin_dir)
            print(string.format(" >>> Quick check: as=%s, gcc=%s, g++=%s, objcopy=%s",
                has("arm-none-eabi-as") and "ok" or "missing",
                has("arm-none-eabi-gcc") and "ok" or "missing",
                has("arm-none-eabi-g++") and "ok" or "missing",
                has("arm-none-eabi-objcopy") and "ok" or "missing"))

            print(">>> Registered bindir/sdkdir.")
        else
            print(">>> Warning: ARM Toolchain not found! Please check PATH or set ARM_GCC_SDK.")
        end
    end)

    set_toolset("cc", "arm-none-eabi-gcc")
    set_toolset("cxx", "arm-none-eabi-g++")
    set_toolset("as", "arm-none-eabi-gcc")
    set_toolset("ld", "arm-none-eabi-gcc")
    set_toolset("ar", "arm-none-eabi-ar")
    set_toolset("objcopy", "arm-none-eabi-objcopy")
    set_toolset("size", "arm-none-eabi-size")

toolchain_end()

rule("util.convert_bin_hex")
    after_build(function (target)
        import("lib.detect.find_tool")

        local targetfile = target:targetfile()

        local objcopy = nil
        local toolchain = nil

        for _, tc in ipairs(target:toolchains()) do
            if tc:name() == "arm-none-eabi-custom" or tc:name() == "arm-none-eabi" then
                toolchain = tc
                break
            end
        end
        if toolchain then
            local bindir = toolchain:bindir()
            if bindir then
                local ext = is_host("windows") and ".exe" or ""
                local prog = path.join(bindir, "arm-none-eabi-objcopy" .. ext)
                if os.isfile(prog) then
                    objcopy = { program = prog }
                end
            end
        end

        if not objcopy then
            objcopy = find_tool("arm-none-eabi-objcopy", {program = "arm-none-eabi-objcopy"})
        end

        if not objcopy then
            print(">>> Error: arm-none-eabi-objcopy not found! Cannot generate hex/bin.")
            return
        end

        local objcopy_prog = objcopy.program

        local function exists(p) return p and os.isfile(p) end
        local elf_file_candidates = {
            targetfile,
            targetfile .. ".elf",
            path.join(path.directory(targetfile), path.filename(targetfile) .. ".elf")
        }
        local elf_file = nil
        for _, c in ipairs(elf_file_candidates) do
            if exists(c) then
                elf_file = c
                break
            end
        end

        if not elf_file then
            local dir = path.directory(targetfile)
            local base = path.filename(targetfile)
            for _, f in ipairs(os.match(path.join(dir, base .. "*.elf")) or {}) do
                if exists(f) then
                    elf_file = f
                    break
                end
            end
        end

        if not elf_file and exists(targetfile) then
            elf_file = targetfile
        end

        if not elf_file or not exists(elf_file) then
            print(string.format(">>> Error: cannot locate ELF for target (tried: %s). Skipping objcopy.", table.concat(elf_file_candidates, ", ")))
            return
        end

        local out_base = elf_file:gsub("%.elf$", "")
        local bin_file = out_base .. ".bin"
        local hex_file = out_base .. ".hex"

        print(">>> Using ELF: " .. elf_file)
        print(">>> Generating BIN: " .. bin_file)

        os.vrunv(objcopy_prog, {"-O", "binary", elf_file, bin_file})

        print(">>> Generating HEX: " .. hex_file)
        os.vrunv(objcopy_prog, {"-O", "ihex", elf_file, hex_file})

        local artifacts_dir = path.join(os.projectdir(), "build", "artifacts")
        if not os.isdir(artifacts_dir) then os.mkdir(artifacts_dir) end
        local preserved_elf = path.join(artifacts_dir, path.filename(elf_file))
        os.cp(elf_file, preserved_elf)
        print(">>> ELF preserved to: " .. preserved_elf)

        print(">>> Build Artifacts Generated Successfully!")
    end)
rule_end()
