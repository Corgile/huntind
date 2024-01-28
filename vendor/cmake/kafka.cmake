# 设置是否编译 librdkafka
set(RDKAFKA_BUILD_STATIC ON CACHE BOOL "Build librdkafka")
set(RDKAFKA_BUILD_EXAMPLES OFF CACHE BOOL "Dont Build examples")
set(RDKAFKA_BUILD_TESTS OFF CACHE BOOL "Dont Build tests")
add_subdirectory(${VENDOR_PATH}/librdkafka-2.3.0)