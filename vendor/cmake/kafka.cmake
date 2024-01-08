if (NOT EXISTS ${VENDOR_PATH}/librdkafka)
    set(BRANCH v2.2.0)
    message(STATUS "拉取 confluentinc/librdkafka ...")
    execute_process(COMMAND git clone -b ${BRANCH} https://github.com/confluentinc/librdkafka.git ${VENDOR_PATH}/librdkafka)
    message(STATUS "confluentinc/librdkafka switched to ${BRANCH}")
endif ()

# 设置是否编译 librdkafka
set(RDKAFKA_BUILD_STATIC ON CACHE BOOL "Build librdkafka")
set(RDKAFKA_BUILD_EXAMPLES OFF CACHE BOOL "Dont Build examples")
set(RDKAFKA_BUILD_TESTS OFF CACHE BOOL "Dont Build tests")
add_subdirectory(${VENDOR_PATH}/librdkafka)