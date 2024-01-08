if (NOT EXISTS ${VENDOR_PATH}/libpcap)
    set(BRANCH libpcap-1.10.4)
    # 拉取 the-tcpdump-group/libpcap.git 子模块并切换到指定分支
    message(STATUS "拉取 the-tcpdump-group/libpcap.git ...")
    execute_process(COMMAND git clone -b ${BRANCH} https://github.com/the-tcpdump-group/libpcap.git ${VENDOR_PATH}/libpcap)
    message(STATUS "the-tcpdump-group/libpcap.git has switched to ${BRANCH}")
endif ()

# 指定 libpcap 的源代码位置
set(LIBPCAP_SOURCE_DIR ${VENDOR_PATH}/libpcap)
# 包含 libpcap 的源代码目录
add_subdirectory(${LIBPCAP_SOURCE_DIR} ${CMAKE_BINARY_DIR}/mypcap)
# 重命名目标库
set_target_properties(pcap PROPERTIES OUTPUT_NAME mypcap)
include_directories(${LIBPCAP_SOURCE_DIR})