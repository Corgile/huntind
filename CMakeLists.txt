CMAKE_MINIMUM_REQUIRED(VERSION 3.25)
PROJECT(hound-torch)
SET(CMAKE_CXX_STANDARD 20)
SET(TARGET hd-torch-kafka-mt)

INCLUDE(cmake/ansi.cmake)

#@format:off
OPTION(HD_USE_CUDA      "Use CUDA"              ON)
OPTION(HD_USE_JE_MALLOC "Use jemalloc"          OFF)
OPTION(HD_USE_TC_MALLOC "Use tcmalloc"          OFF)
OPTION(HD_ENABLE_LOGS   "enable log file"       ON)
OPTION(HD_CONSOLE_LOG   "enable log console"    OFF)
OPTION(HD_USE_THREADS   "Use Threads::Threads"  OFF)
#@format:on
INCLUDE(cmake/log-level.cmake)

IF(HD_USE_CUDA)
  MESSAGE(STATUS "${BoldBlue}使用 CUDA: ${BoldCyan}${HD_USE_CUDA}${ColorReset}")
  SET(CMAKE_CUDA_ARCHITECTURES 86)
  SET(CMAKE_CUDA_COMPILER /usr/local/cuda/bin/nvcc)
  SET(CAFFE2_USE_CUDNN ON)
  ADD_COMPILE_DEFINITIONS(USE_CUDA)
ENDIF()

SET(PYTHON_EXECUTABLE $ENV{YOUR_PREFFRED_PYTHON_EXECUTABLE};${PYTHON_EXECUTABLE})
FIND_PACKAGE(Torch REQUIRED PATHS $ENV{TORCH_PATH})
MESSAGE(STATUS "${BoldGreen}Torch 链接库:  ${ColorReset}${TORCH_LIBRARIES}")
MESSAGE(STATUS "${BoldGreen}Torch 头文件:  ${ColorReset}${TORCH_INCLUDE_DIRS}")

IF(NOT HD_USE_THREADS)
  FIND_PACKAGE(RdKafka CONFIG REQUIRED)
  GET_TARGET_PROPERTY(RdKafka__INCLUDES RdKafka::rdkafka++ INTERFACE_INCLUDE_DIRECTORIES)
  GET_TARGET_PROPERTY(RdKafka_LIBRARIES RdKafka::rdkafka++ INTERFACE_LINK_LIBRARIES)
  MESSAGE(STATUS "${BoldGreen}RdKafka 链接库:  ${ColorReset}${RdKafka_LIBRARIES}")
  MESSAGE(STATUS "${BoldGreen}RdKafka 头文件:  ${ColorReset}${RdKafka__INCLUDES}")
ENDIF()

INCLUDE_DIRECTORIES(
    ${RdKafka__INCLUDES}
    ${TORCH_INCLUDE_DIRS}
    include
    include/yalanting/include
    include/yalanting/include/ylt/thirdparty
)

ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/source)
ADD_EXECUTABLE(${TARGET} main.cpp ${SOURCE_FILES})

IF(HD_USE_THREADS)
  MESSAGE(STATUS "${BoldBlue}Using Threads::Threads: ${BoldCyan}${HD_USE_THREADS}${ColorReset}")
  FIND_PACKAGE(Threads REQUIRED)
  TARGET_LINK_LIBRARIES(${TARGET} PRIVATE Threads::Threads)
ENDIF()

TARGET_LINK_LIBRARIES(${TARGET} PRIVATE hd_source)
MESSAGE(STATUS "${BoldRed}Target:  ${TARGET}${ColorReset}")
