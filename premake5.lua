workspace "VisualCrypto"
    startproject "VisualCrypto"
    architecture "x64"

    configurations
    {
        "Debug",
        "Release"
    }

tdir = "bin/%{cfg.buildcfg}/%{prj.name}"
odir = "bin-obj/%{cfg.buildcfg}/%{prj.name}"
-- External Dependencies
externals = {}
externals["sdl2"] = "external/sdl2"
externals["imgui"] = "external/imgui"
--externals["spdlog"] = "external/spdlog"
externals["glad"] = "external/glad"
--externals["glm"] = "external/glm"
externals["stb"] = "external/stb"
--externals["yaml"] = "external/yaml"
--externals["perlinNoise"] = "external/perlinNoise"

-- process glad before anything else
include"external/glad"

--include "external/yaml"

project "VisualCrypto"
    location "VisualCrypto"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"
    
    flags
    {
        "FatalWarnings"
    }

    targetdir(tdir)
    objdir(odir)
    
    sysincludedirs
    {
        "%{prj.name}/include",
        "%{externals.sdl2}/include",
        "%{externals.imgui}/include",
       -- "%{externals.spdlog}/include",
        "%{externals.glad}/include",
       -- "%{externals.glm}/include",
          "%{externals.stb}/include"
       -- "%{externals.yaml}/include",
       -- "%{externals.perlinNoise}/include"
    }

    libdirs
    {
        "%{externals.sdl2}/lib",
    }

    links
    {
        "glad",
      --  "yaml",
        "SDL2.lib"
    }

    files
    {
        "%{prj.name}/include/**.h",    
        "%{prj.name}/include/**.cpp",   
        "%{prj.name}/include/**.hpp",   
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
        "%{prj.name}/src/**.glsl",
        "%{prj.name}/**.natvis",
        "%{externals.imgui}/include/**.cpp"       
    }
    
    defines
    {
        "GLFW_INCLUDE_NONE" -- ensure glad doesn't include glfw
    }
    systemversion "latest"
    
    filter {"configurations:Debug"}
        
        defines
        {
            "VC_CONFIG_DEBUG"
        }
        runtime "Debug"
        symbols "on"
    
    filter {"configurations:Release"}
        
        defines
        {
            "VC_CONFIG_RELEASE"
        }
        runtime "Release"
        symbols "off"
        optimize "on"
