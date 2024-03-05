INCLUDE(FetchContent)

IF(NOT EXISTS ${VENDOR_PATH}/dbg-macro)
    SET(BRANCH master)
    MESSAGE(STATUS "拉取 sharkdp/dbg-macro")
    # 拉取 dbg-macro 子模块并切换到指定分支
    EXECUTE_PROCESS(COMMAND git clone -b ${BRANCH} https://github.com/sharkdp/dbg-macro.git ${VENDOR_PATH}/dbg-macro)
ENDIF()
ADD_COMPILE_DEFINITIONS(DBG_MACRO_NO_WARNING)
#FetchContent_Declare(dbg_macro GIT_REPOSITORY https://github.com/sharkdp/dbg-macro.git GIT_TAG master)
FETCHCONTENT_DECLARE(dbg_macro SOURCE_DIR ${VENDOR_PATH}/dbg-macro)
FETCHCONTENT_MAKEAVAILABLE(dbg_macro)
