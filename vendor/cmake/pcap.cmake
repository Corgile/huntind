IF (NOT EXISTS ${VENDOR_PATH}/libpcap)
    SET(BRANCH libpcap-1.10.4)
    # 拉取 the-tcpdump-group/libpcap.git 子模块并切换到指定分支
    MESSAGE(STATUS "拉取 the-tcpdump-group/libpcap.git ...")
    EXECUTE_PROCESS(COMMAND git clone -b ${BRANCH} https://github.com/the-tcpdump-group/libpcap.git ${VENDOR_PATH}/libpcap)
    MESSAGE(STATUS "the-tcpdump-group/libpcap.git has switched to ${BRANCH}")
ENDIF ()

# 指定 libpcap 的源代码位置
SET(LIBPCAP_SOURCE_DIR ${VENDOR_PATH}/libpcap)
# 包含 libpcap 的源代码目录
ADD_SUBDIRECTORY(${LIBPCAP_SOURCE_DIR} ${CMAKE_BINARY_DIR}/mypcap)
# 重命名目标库
SET_DIRECTORY_PROPERTIES(pcap PROPERTIES OUTPUT_NAME mypcap)
INCLUDE_DIRECTORIES(${LIBPCAP_SOURCE_DIR})