
add_library(TraceClient STATIC)

target_compile_options(TraceClient 
    PRIVATE 
        -fno-exceptions
        -fno-rtti
        -Wall 
        #-Werror
)

target_include_directories(TraceClient INTERFACE "${PROJECT_SOURCE_DIR}/src")
target_sources(TraceClient PRIVATE "${PROJECT_SOURCE_DIR}/src/TraceClient/TraceClient.cpp")

target_link_libraries(TraceClient PUBLIC TraceCommon)

# Platform specific macros
target_compile_definitions(TraceClient PRIVATE
    $<$<PLATFORM_ID:Windows>:TS_PLATFORM_WINDOWS>
    $<$<PLATFORM_ID:Linux>:TS_PLATFORM_LINUX>
)


