IF(NOT EXISTS ${VENDOR_PATH}/librdkafka)
    SET(BRANCH v2.2.0)
    MESSAGE(STATUS "拉取 confluentinc/librdkafka ...")
    EXECUTE_PROCESS(COMMAND git clone -b ${BRANCH} https://github.com/confluentinc/librdkafka.git ${VENDOR_PATH}/librdkafka)
    MESSAGE(STATUS "confluentinc/librdkafka switched to ${BRANCH}")
ENDIF()

# 设置是否编译 librdkafka
SET(RDKAFKA_BUILD_STATIC ON CACHE BOOL "Build librdkafka")
SET(RDKAFKA_BUILD_EXAMPLES OFF CACHE BOOL "Dont Build examples")
SET(RDKAFKA_BUILD_TESTS OFF CACHE BOOL "Dont Build tests")
ADD_SUBDIRECTORY(${VENDOR_PATH}/librdkafka)