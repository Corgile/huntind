INCLUDE(FetchContent)

IF(NOT EXISTS ${VENDOR_PATH}/nlohmann-json)
    SET(BRANCH v3.11.2)
    # 拉取 nlohmann/json 子模块并切换到指定分支
    MESSAGE(STATUS "拉取 nlohmann/json ...")
    EXECUTE_PROCESS(COMMAND git clone -b ${BRANCH} https://github.com/nlohmann/json.git ${VENDOR_PATH}/nlohmann-json)
    MESSAGE(STATUS "nlohmann/json switched to ${BRANCH}")
ENDIF()

FETCHCONTENT_DECLARE(nlohmann_json SOURCE_DIR ${VENDOR_PATH}/nlohmann-json)
FETCHCONTENT_MAKEAVAILABLE(nlohmann_json)
