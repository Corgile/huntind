CMAKE_MINIMUM_REQUIRED(VERSION 3.25)
PROJECT(encoding-test)
SET(CMAKE_CXX_STANDARD 20)
SET(ENCODING_TEST_TARGET encoding-test)

MESSAGE(STATUS "测试程序:  ${ENCODING_TEST_TARGET}")
MESSAGE(STATUS "链接编码:  ${ENCODER_LIBRARY}")

ADD_EXECUTABLE(${ENCODING_TEST_TARGET} main.cpp)
TARGET_LINK_LIBRARIES(${ENCODING_TEST_TARGET} PRIVATE ${ENCODER_LIBRARY})