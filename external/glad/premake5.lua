project "glad"
    kind "StaticLib"
    language "C"
    staticruntime "on"
    

    targetdir(tdir)
    objdir(odir)
    sysincludedirs
    {
        "include/"
    }

    files
    {
        "include/**.h",   
        "src/**.c"
    }
    
    filter { "system:windows"}
        systemversion "latest"
    
    filter { "system:macosx"}
        xcodebuildsettings
        {
            ["MACOSX_DEPLOYMENT_TARGET"] = "10.15",
            ["UseModernBuildSystem"] = "NO"
        }
    
    filter {"configurations:Debug"}
        runtime "Debug"
        symbols "on"
    
    filter {"configurations:Release"}
        runtime "Release"
        symbols "off"
        optimize "on"