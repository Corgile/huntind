IF(NOT WIN32)
    STRING(ASCII 27 Esc)
    SET(ColorReset "${Esc}[m")
    SET(ColorBold  "${Esc}[1m")
    SET(Red         "${Esc}[31m")
    SET(Green       "${Esc}[32m")
    SET(Yellow      "${Esc}[33m")
    SET(Blue        "${Esc}[34m")
    SET(Magenta     "${Esc}[35m")
    SET(Cyan        "${Esc}[36m")
    SET(White       "${Esc}[37m")
    SET(BoldRed     "${Esc}[1;31m")
    SET(BoldGreen   "${Esc}[1;32m")
    SET(BoldYellow  "${Esc}[1;33m")
    SET(BoldBlue    "${Esc}[1;34m")
    SET(BoldMagenta "${Esc}[1;35m")
    SET(BoldCyan    "${Esc}[1;36m")
    SET(BoldWhite   "${Esc}[1;38m")
ENDIF()