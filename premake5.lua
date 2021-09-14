
workspace "TraceServer"
   
    configurations { "Debug", "Release" }
    architecture "x86_64"
    platforms { "Win64", "Linux" }
    cppdialect "C++17"
    location "build"
    language "C++"
    targetdir "bin"
    targetsuffix "_%{cfg.buildcfg}"
    objdir "%{prj.location}/intermediates/%{prj.name}_%{cfg.platform}_%{cfg.buildcfg}"

    flags
    {
        "MultiProcessorCompile"
    }

    rtti "Off"
    exceptionhandling "Off"
    warnings "Extra"
    staticruntime "off"
    defaultplatform "Win64"

    -- Platforms setup
    filter { "platforms:Win64" }
        system "Windows"
        defines "TS_PLATFORM_WINDOWS"
        systemversion "latest" 

    filter { "platforms:Linux" }
        system "linux"
        defines "TS_PLATFORM_LINUX"
        defines "LINUX"

    -- Configurations setup
    filter "configurations:Debug"
        symbols "On"
        runtime "Debug"
        defines 
        { 
            "DEBUG"
        }

    filter "configurations:Release"
        symbols "Off"
        runtime "Release"
        optimize "Speed"
                
        defines 
        { 
            "NDEBUG"
        }

project "Server"
    kind "WindowedApp"
    files { "src/Server/**.h", "src/Server/**.cpp" }

    -- TODO: 3rd is completely unhandled

    includedirs 
    {
        "src/Server",
    }

project "Client"
    kind "StaticLib"
    files { "src/Client/**.h", "src/Client/**.cpp" }