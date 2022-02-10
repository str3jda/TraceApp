
add_library(TraceClient STATIC)

target_include_directories(TraceClient INTERFACE "${PROJECT_SOURCE_DIR}/src/TraceClient")
target_sources(TraceClient PRIVATE "${PROJECT_SOURCE_DIR}/src/TraceClient/TraceClient.cpp")

# Platform specific macros
target_compile_definitions(TraceClient PRIVATE
    $<$<PLATFORM_ID:Windows>:TS_PLATFORM_WINDOWS>
    $<$<PLATFORM_ID:Linux>:TS_PLATFORM_LINUX>
)


