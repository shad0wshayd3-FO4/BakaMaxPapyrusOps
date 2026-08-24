-- include subprojects
includes("lib/commonlibf4")

-- set project constants
set_project("BakaMaxPapyrusOps")
set_version("4.0.0")
set_license("GPL-3.0")
set_languages("c++23")
set_warnings("allextra")

-- add common rules
add_rules("mode.debug", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")

-- set configs
set_config("commonlib_ini", true)
set_config("commonlib_xbyak", true)

-- define targets
target("BakaMaxPapyrusOps")
    add_rules("commonlibf4.plugin", {
        name = "BakaMaxPapyrusOps",
        author = "shad0wshayd3"
    })

    -- add src files
    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/pch.h")

    -- add extra files
    add_extrafiles(".clang-format")
