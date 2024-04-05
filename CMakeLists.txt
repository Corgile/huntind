CMAKE_MINIMUM_REQUIRED(VERSION 3.25)
PROJECT(hound-torch)
SET(CMAKE_CXX_STANDARD 20)
SET(TARGET hd-torch-kafka)

INCLUDE(cmake/ansi.cmake)
ADD_COMPILE_DEFINITIONS(HD_LOG_DEBUG)

OPTION(USE_CUDA "Use CUDA" ON)

IF(USE_CUDA)
    MESSAGE(STATUS "${BoldBlue}Using CUDA: ${BoldGreen}YES${ColorReset}")
    SET(CMAKE_CUDA_ARCHITECTURES 86)
    SET(CMAKE_CUDA_COMPILER /usr/local/cuda/bin/nvcc)
    SET(CAFFE2_USE_CUDNN ON)
    ADD_COMPILE_DEFINITIONS(USE_CUDA)
ENDIF()

LIST(APPEND CMAKE_PREFIX_PATH $ENV{TORCH_PATH})
FIND_PACKAGE(Torch REQUIRED)
MESSAGE(STATUS "${BoldGreen}Torch 链接库:  ${ColorReset}${TORCH_LIBRARIES}")
MESSAGE(STATUS "${BoldGreen}Torch 头文件:  ${ColorReset}${TORCH_INCLUDE_DIRS}")

INCLUDE_DIRECTORIES(
        ${TORCH_INCLUDE_DIRS}
        include
        include/yalanting/include
        include/yalanting/include/ylt/thirdparty
)

ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/source)
ADD_EXECUTABLE(${TARGET} main.cpp ${SOURCE_FILES})

OPTION(USE_THREADS "Use Threads::Threads" OFF)
IF(USE_THREADS)
    MESSAGE(STATUS "${BoldYellow}Using Threads::Threads: ${BoldGreen}YES${ColorReset}")
    FIND_PACKAGE(Threads REQUIRED)
    TARGET_LINK_LIBRARIES(${TARGET} PRIVATE Threads::Threads)
ENDIF()
TARGET_LINK_LIBRARIES(${TARGET} PRIVATE hd_source)
MESSAGE(STATUS "${BoldRed}Target:  ${TARGET}${ColorReset}")
