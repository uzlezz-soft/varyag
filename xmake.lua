option("vvalidation")
    add_defines("VG_VALIDATION")

target("varyag")
    set_kind("shared")
    add_defines("VG_EXPORT")

    set_languages("cxx20")
    add_includedirs("include", { public = true })
    add_headerfiles("include/varyag.h")
    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    set_options("vvalidation")

    add_files("vendor/src/**.cpp")
    add_includedirs("vendor/include")
    add_includedirs("vendor/include/agilitysdk")
    add_includedirs("vendor/include/agilitysdk/d3dx12")

    set_symbols("debug")

includes("samples")