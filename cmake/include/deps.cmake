MACRO (CMAKE_DEPS_INIT)
    SET(CMAKE_DEPS_FILE ${ARCADIA_BUILD_ROOT}/cmake_deps)
    FILE(REMOVE ${CMAKE_DEPS_FILE})
ENDMACRO (CMAKE_DEPS_INIT)

MACRO (CMAKE_DEPS_TARGET)
    string(REPLACE ${ARCADIA_ROOT}/ "" __thispath_ ${CMAKE_CURRENT_SOURCE_DIR})
    # DLOG("APPEND TO [${CMAKE_DEPS_FILE}]: ${__sources_1}")

    SET(TYPE ${SAVEDTYPE})
    IF (DUMMY_SHARED_PRJNAME)
        SET(TYPE "SHLIB")
    ENDIF (DUMMY_SHARED_PRJNAME)

    FILE(APPEND ${CMAKE_DEPS_FILE} "module: ${__thispath_} ${TYPE} ${REALPRJNAME} {\n")
    # FILE(APPEND ${CMAKE_DEPS_FILE} "${__thispath_} ${CURPROJTYPES} ${REALPRJNAME}\n")
    FILE(APPEND ${CMAKE_DEPS_FILE} "   sources: ${__sources_1}\n")
    FILE(APPEND ${CMAKE_DEPS_FILE} "   peerdir: ${PEERDIR}")

    LIST(LENGTH THISPROJECTDEPENDSDIRSNAMES len)
    IF (len)
         FILE(APPEND ${CMAKE_DEPS_FILE} ";${THISPROJECTDEPENDSDIRSNAMES}\n")
         FILE(APPEND ${CMAKE_DEPS_FILE} "   buildafter: ${THISPROJECTDEPENDSDIRSNAMES}\n")
    ELSE (len)
         FILE(APPEND ${CMAKE_DEPS_FILE} "\n")
    ENDIF (len)

    FILE(APPEND ${CMAKE_DEPS_FILE} "   sourcedirs: ${SRCDIR}\n")
    FILE(APPEND ${CMAKE_DEPS_FILE} "}\n")
ENDMACRO (CMAKE_DEPS_TARGET)

MACRO (CMAKE_DEPS_FINISH)
    FILE(APPEND ${CMAKE_DEPS_FILE} "end\n")
ENDMACRO (CMAKE_DEPS_FINISH)

