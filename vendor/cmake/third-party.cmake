# TODO 根据build类型添加所需依赖
include(ansi-color)
include(json)
include(dbg-macro)
if (INCLUDE_KAFKA)
#    include(kafka)
    include(kafka-cpp-api)
endif ()

