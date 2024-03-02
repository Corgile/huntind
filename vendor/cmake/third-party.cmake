# TODO 根据build类型添加所需依赖
include(ansi-color)
if (INCLUDE_KAFKA)
#    include(kafka)
    include(kafka-cpp-api)
endif ()

