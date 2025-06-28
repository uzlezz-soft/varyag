add_requires("glfw", "mimalloc", "glm", "assimp", "freeimage", "directxshadercompiler")

add_rules("mode.debug", "mode.release")

target("model_viewer")
    set_kind("binary")
    set_languages("cxx20")

    add_includedirs("model_viewer/include")
    add_headerfiles("model_viewer/include/**.h")
    add_files("model_viewer/src/**.cpp")
    add_deps("varyag")

    set_rundir("$(projectdir)/samples/model_viewer/data")
    set_configdir("$(buildir)/$(plat)/$(arch)/$(mode)/shaders")
    add_configfiles("model_viewer/data/shaders/*.hlsl", {onlycopy = true})
    add_configfiles("model_viewer/data/shaders/*.hlsli", {onlycopy = true})

    set_symbols("debug")
    add_packages("glfw", "mimalloc", "glm", "assimp", "freeimage", "directxshadercompiler", "volk")