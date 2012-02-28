SET_IF_NOTSET(MD5LIB -lmd)

SET(INVALID_TOOLS)

INCLUDE(${ARCADIA_ROOT}/cmake/icc/icc.cmake)

IF (USE_CLANG)
    SET(CLDRV "${__cmake_incdir_}/../scripts/cldriver")

    SET(AR ${CLDRV})
    SET(CMAKE_AR ${AR})

    SET(RANLIB ${CLDRV})
    SET(CMAKE_RANLIB ${RANLIB})

    SET(USE_STATIC_CPP_RUNTIME no)
ENDIF (USE_CLANG)

IF (LINUX)
    EXECUTE_PROCESS(COMMAND uname -r
        OUTPUT_VARIABLE LINUX_KERNEL
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    STRING(REGEX REPLACE "\(...\).*" LINUX_KERNEL "\\1" ${LINUX_KERNEL})

    SET_IF_NOTSET(PKG_CREATE rpm)
    SET_IF_NOTSET(RPMBUILD rpm)
    SET(MD5LIB -lcrypt)
    SET(LIBRT -lrt)
    SET(LIBRESOLV -lresolv)
    SET_IF_NOTSET(APACHE            /usr/include/apache-1.3)
    SET_IF_NOTSET(APACHE_INCLUDE    /usr/include/apache-1.3)
ELSEIF (SUN)
    #SET_IF_NOTSET(PUBROOM /opt/arcadia/room)
    SET_IF_NOTSET(PKG_CREATE pkgmk)
    SET_IF_NOTSET(THREADLIB -lpthread)
    IF (SUN)
        SET(INSTALL     /usr/ucb/install)
        SET_IF_NOTSET(AWK   /usr/bin/awk)
        SET_IF_NOTSET(TAIL  /usr/xpg4/bin/tail)
        SET_IF_NOTSET(BISON /usr/local/bin/bison)
    ENDIF (SUN)
ELSEIF (CYGWIN)
    SET_IF_NOTSET(THREADLIB -lpthread)
ELSEIF (DARWIN)
    SET_IF_NOTSET(THREADLIB -lpthread)
    SET(LIBRESOLV -lresolv)
ELSEIF (WIN32)
    # TODO: Add necessary defs
    SET_IF_NOTSET(RM del)
    SET_IF_NOTSET(RM_FORCEFLAG "/F")
    SET_IF_NOTSET(CAT type)
    SET_IF_NOTSET(MV move)
    SET_IF_NOTSET(MV_FORCEFLAG "/Y")
ELSEIF (FREEBSD)
    IF (ARCOS)
        SET_IF_NOTSET(THREADLIB -kthread)
    ENDIF (ARCOS)
    EXECUTE_PROCESS(COMMAND uname -r
        OUTPUT_VARIABLE FREEBSD_VER
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    STRING(REGEX REPLACE "\(.\).*" "\\1" FREEBSD_VER ${FREEBSD_VER})

    EXECUTE_PROCESS(COMMAND uname -r
        OUTPUT_VARIABLE FREEBSD_VER_MINOR
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    STRING(REGEX REPLACE "..\(.\).*" "\\1" FREEBSD_VER_MINOR ${FREEBSD_VER_MINOR})

    SET_IF_NOTSET(PKG_CREATE pkg_create)
    SET(LDFLAGS "${LDFLAGS} -Wl,-E")
    SET_IF_NOTSET(VALGRIND_DIR /usr/local/valgrind/)
    SET_IF_NOTSET(APACHE /large/old/usr/place/apache-nl/apache_1.3.29rusPL30.19/src)
    IF (DEFINED FREEBSD_VER AND "${FREEBSD_VER}" MATCHES "4")
        SET_IF_NOTSET(GCOV gcov34)
    ENDIF (DEFINED FREEBSD_VER AND "${FREEBSD_VER}" MATCHES "4")
    SET(CTAGS_PRG exctags)
ENDIF (LINUX)

IF (SUN)
    SET(SONAME -h)
    SET(TSORT_FLAGS)
ELSEIF (CYGWIN)
    SET(SONAME -h)
    SET(TSORT_FLAGS)
ELSE (SUN) #linux/freebsd
    SET(SONAME -soname)
    SET(TSORT_FLAGS -q)
ENDIF (SUN)

SET_IF_NOTSET(APACHE_INCLUDE ${APACHE}/include)

IF(ARCOS)
    SET_IF_NOTSET(PERL      perl5.8.8)
ELSE(ARCOS)
    SET_IF_NOTSET(PERL      perl)
ENDIF(ARCOS)

IF (PERL)
    IF (NOT PERL_VERSION)
        EXECUTE_PROCESS(COMMAND ${PERL} -V:version
            OUTPUT_VARIABLE PERL_VERSION
            ERROR_VARIABLE PERL_VERSION_ERR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_STRIP_TRAILING_WHITESPACE
        )
        IF (PERL_VERSION)
            STRING(REPLACE "\n" " " PERL_VERSION ${PERL_VERSION})
            STRING(REGEX MATCH "version='[0-9\\.]*'" PERL_VERSION ${PERL_VERSION})
            STRING(REPLACE "version=" "" PERL_VERSION ${PERL_VERSION})
            STRING(REPLACE "'" "" PERL_VERSION ${PERL_VERSION})
            STRING(REPLACE "." ";" PERL_VERSION ${PERL_VERSION})
            SET(__perl_ver_ 0)
            FOREACH(__item_ ${PERL_VERSION})
                MATH(EXPR __perl_ver_ "${__perl_ver_}*100+${__item_}")
            ENDFOREACH(__item_)
            SET(PERL_VERSION ${__perl_ver_})
        ELSE (PERL_VERSION)
            SET(PERL_VERSION NOTFOUND)
        ENDIF (PERL_VERSION)
        DEBUGMESSAGE(1 "PERL_VERSION is [${PERL_VERSION}]")
    ENDIF (NOT PERL_VERSION)
ENDIF (PERL)

SET_IF_NOTSET(BISON     bison)
SET_IF_NOTSET(GCOV      gcov)
SET_IF_NOTSET(GCOV_OPTIONS  --long-file-names)
SET_IF_NOTSET(RM rm)
SET_IF_NOTSET(RM_FORCEFLAG "-f")
SET_IF_NOTSET(CAT cat)
SET_IF_NOTSET(MV mv)
SET_IF_NOTSET(MV_FORCEFLAG "-f")
SET_IF_NOTSET(CTAGS_PRG ctags)

SET_IF_NOTSET(AR ar)
SET_IF_NOTSET(RANLIB ranlib)

SET_IF_NOTSET(PYTHON python)
IF (NOT PYTHON_VERSION)
    IF (WIN32)
        SET(PYTHON_VERSION 2.5)
    ELSE (WIN32)
        EXECUTE_PROCESS(COMMAND ${PYTHON} -V
        OUTPUT_VARIABLE PYTHON_VERSION
        ERROR_VARIABLE PYTHON_VERSION_ERR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE
        )
        SET(PYTHON_VERSION "${PYTHON_VERSION}${PYTHON_VERSION_ERR}")
        IF (PYTHON_VERSION)
            STRING(REGEX REPLACE "^[^ ]* \([^.]*.[^.]*\).*" "\\1" PYTHON_VERSION "${PYTHON_VERSION}")
        ELSE (PYTHON_VERSION)
            MESSAGE(STATUS "\${PYTHON_VERSION} is empty. Please check if \${PYTHON} variable is set properly")
        ENDIF (PYTHON_VERSION)
    ENDIF (WIN32)
ENDIF (NOT PYTHON_VERSION)
IF (WIN32)
	SET_IF_NOTSET(PYTHON_INCLUDE C:/Python/include)
ELSEIF (LINUX OR DARWIN)
	SET_IF_NOTSET(PYTHON_INCLUDE /usr/include/python${PYTHON_VERSION})
ELSE (WIN32) # sun/freebsd
	SET_IF_NOTSET(PYTHON_INCLUDE /usr/local/include/python${PYTHON_VERSION})
ENDIF (WIN32)

IF (NOT CCVERS)
    IF (WIN32)
        SET(CCVERS 0)
    ELSE (WIN32)
        EXECUTE_PROCESS(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
        COMMAND awk -F. "{print $1*10000+$2*100+$3}"
        OUTPUT_VARIABLE CCVERS
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    ENDIF (WIN32)
ENDIF (NOT CCVERS)

INCLUDE(CheckSymbolExists)
CHECK_SYMBOL_EXISTS(__PATHSCALE__ "" IS_PATHSCALE)

INCLUDE(TestCXXAcceptsFlag)

IF (BUILD_FOR_PRODUCTION)
    # Build production binaries with preset CPU features.
    MESSAGE(STATUS "BUILD_FOR_PRODUCTION enabled")
    SET(IS_MNOSSE_SUPPORTED yes)
    SET(IS_MSSE_SUPPORTED yes)
    SET(IS_MSSE2_SUPPORTED yes)
    SET(IS_MSSE3_SUPPORTED yes)
    SET(IS_MSSSE3_SUPPORTED no)
ELSE (BUILD_FOR_PRODUCTION)
    IF (IS_PATHSCALE)
        # pathscale rejects these sse flags without a proper -march
        CHECK_CXX_ACCEPTS_FLAG("-mssse3 -march=core" IS_MSSSE3_SUPPORTED)
        CHECK_CXX_ACCEPTS_FLAG("-msse3 -march=core" IS_MSSE3_SUPPORTED)
        CHECK_CXX_ACCEPTS_FLAG("-msse2 -march=core" IS_MSSE2_SUPPORTED)
    ELSE (IS_PATHSCALE)
        CHECK_CXX_ACCEPTS_FLAG("-mssse3" IS_MSSSE3_SUPPORTED)
        CHECK_CXX_ACCEPTS_FLAG("-msse3" IS_MSSE3_SUPPORTED)
        CHECK_CXX_ACCEPTS_FLAG("-msse2" IS_MSSE2_SUPPORTED)
    ENDIF (IS_PATHSCALE)
    CHECK_CXX_ACCEPTS_FLAG("-msse" IS_MSSE_SUPPORTED)
    CHECK_CXX_ACCEPTS_FLAG("-mno-sse" IS_MNOSSE_SUPPORTED)
ENDIF (BUILD_FOR_PRODUCTION)

#
# For USE_STATIC_CPP_RUNTIME
#
IF (ARCOS)
    DEFAULT(GCC_TARGET i386-portbld-freebsd4.11)
    DEFAULT(GCC_FULLLIBDIR    /usr/local/gcc-3.4.6/lib/gcc/i386-portbld-freebsd4.11/3.4.6)
    DEFAULT(GCC_FULLLIBGCCDIR ${GCC_FULLLIBDIR}/gcc/i386-portbld-freebsd4.11/3.4.6)
    DEFAULT(GCC_FULLCRTOBJDIR ${GCC_FULLLIBGCCDIR})
ELSEIF (NOT WIN32)
    SET(__OPTS)
    IF ("${HARDWARE_TYPE}" MATCHES "x86_64")
        SET(__OPTS -m64)
    ENDIF ("${HARDWARE_TYPE}" MATCHES "x86_64")
    IF (NOT DEFINED GCC_FULLLIBDIR)
        EXECUTE_PROCESS(
            COMMAND ${CMAKE_CXX_COMPILER} ${__OPTS} "-print-file-name=libsupc++.a"
            OUTPUT_VARIABLE GCC_FULLLIBDIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        IF (EXISTS "${GCC_FULLLIBDIR}")
            GET_FILENAME_COMPONENT(GCC_FULLLIBDIR "${GCC_FULLLIBDIR}" PATH)
        ENDIF (EXISTS "${GCC_FULLLIBDIR}")
    ENDIF (NOT DEFINED GCC_FULLLIBDIR)

    IF (NOT DEFINED GCC_FULLLIBGCCDIR)
        EXECUTE_PROCESS(
            COMMAND ${CMAKE_CXX_COMPILER} ${__OPTS} "-print-file-name=libgcc.a"
            OUTPUT_VARIABLE GCC_FULLLIBGCCDIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        IF (EXISTS "${GCC_FULLLIBGCCDIR}")
            GET_FILENAME_COMPONENT(GCC_FULLLIBGCCDIR "${GCC_FULLLIBGCCDIR}" PATH)
        ENDIF (EXISTS "${GCC_FULLLIBGCCDIR}")
    ENDIF (NOT DEFINED GCC_FULLLIBGCCDIR)

    IF (NOT DEFINED GCC_FULLCRTOBJDIR)
        EXECUTE_PROCESS(
            COMMAND ${CMAKE_CXX_COMPILER} ${__OPTS} "-print-file-name=crtbegin.o"
            OUTPUT_VARIABLE GCC_FULLCRTOBJDIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        IF (EXISTS "${GCC_FULLCRTOBJDIR}")
            GET_FILENAME_COMPONENT(GCC_FULLCRTOBJDIR "${GCC_FULLCRTOBJDIR}" PATH)
        ELSE (EXISTS "${GCC_FULLCRTOBJDIR}")
            SET(GCC_FULLCRTOBJDIR)
        ENDIF (EXISTS "${GCC_FULLCRTOBJDIR}")
    ENDIF (NOT DEFINED GCC_FULLCRTOBJDIR)

    IF (NOT DEFINED GCC_FULLCRTOBJDIR)
        EXECUTE_PROCESS(
            COMMAND ${CMAKE_CXX_COMPILER} ${__OPTS} "-print-file-name=crt3.o"
            OUTPUT_VARIABLE GCC_FULLCRTOBJDIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        IF (EXISTS "${GCC_FULLCRTOBJDIR}")
            GET_FILENAME_COMPONENT(GCC_FULLCRTOBJDIR "${GCC_FULLCRTOBJDIR}" PATH)
        ENDIF (EXISTS "${GCC_FULLCRTOBJDIR}")
    ENDIF (NOT DEFINED GCC_FULLCRTOBJDIR)
ENDIF (ARCOS)

IF (NOT GCC_FULLLIBDIR AND NOT WIN32)
    IF (LINUX)
        SET_IF_NOTSET(GCC_LIBDIR /usr/lib)
    ELSEIF (NOT GCC_LIBDIR AND FREEBSD)
        EXECUTE_PROCESS(
            COMMAND ${CMAKE_CXX_COMPILER} -v
            ERROR_VARIABLE GCC_LIBDIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        STRING(REGEX MATCH "--libdir=[^ ]* " GCC_LIBDIR "${GCC_LIBDIR}")
        STRING(REGEX REPLACE "--libdir=" "" GCC_LIBDIR "${GCC_LIBDIR}")
        STRING(REGEX REPLACE " " "" GCC_LIBDIR "${GCC_LIBDIR}")
        STRING(REGEX REPLACE "\n" "" GCC_LIBDIR "${GCC_LIBDIR}")
    ENDIF (LINUX)

    IF (NOT DEFINED GCC_TARGET)
        EXECUTE_PROCESS(
            COMMAND ${CMAKE_CXX_COMPILER} -dumpmachine
            OUTPUT_VARIABLE GCC_TARGET
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    ENDIF (NOT DEFINED GCC_TARGET)

    IF (NOT GCC_VERSION)
        EXECUTE_PROCESS(
            COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
            OUTPUT_VARIABLE GCC_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    ENDIF (NOT GCC_VERSION)

    SET(GCC_FULLLIBDIR    ${GCC_LIBDIR}/gcc/${GCC_TARGET}/${GCC_VERSION})
    SET(GCC_FULLCRTOBJDIR ${GCC_FULLLIBDIR})
    SET(GCC_FULLLIBGCCDIR ${GCC_FULLLIBDIR})

    DEBUGMESSAGE(1 "Can't get GCC_FULLLIBDIR from -print-file-name=libsupc++.a. Euristic method gave path[${GCC_FULLLIBDIR}]")
ENDIF (NOT GCC_FULLLIBDIR AND NOT WIN32)

IF (NOT DEFINED NATIVE_INCLUDE_PATH)
    IF (NOT WIN32)
        EXECUTE_PROCESS(
            COMMAND ${CMAKE_CXX_COMPILER} -v "${ARCADIA_ROOT}/cmake/include/fict_exe_source.cpp" -o "${CMAKE_CURRENT_BINARY_DIR}/gcctest.out"
            OUTPUT_QUIET
            ERROR_VARIABLE GCC_OUT
            ERROR_STRIP_TRAILING_WHITESPACE
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        )
        FILE(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/gcctest.out")

        STRING(REGEX REPLACE "\n" " " GCC_OUT "${GCC_OUT}")
        STRING(REGEX REPLACE ".*search starts here:[ \t]*" "" GCC_OUT "${GCC_OUT}")
        STRING(REGEX REPLACE "End of search list.*" "" GCC_OUT "${GCC_OUT}")
        SEPARATE_ARGUMENTS(GCC_OUT)

        FOREACH (__item__ ${GCC_OUT})
            IF (NOT DEFINED __ret__ AND EXISTS "${__item__}/string")
                SET(__ret__ "${__item__}")
            ENDIF (NOT DEFINED __ret__ AND EXISTS "${__item__}/string")
        ENDFOREACH (__item__ ${GCC_OUT})

        IF (NOT DEFINED __ret__)
            MESSAGE(SEND_ERROR "tools.cmake: Can't get gcc native include path")
        ENDIF (NOT DEFINED __ret__)

        SET(NATIVE_INCLUDE_PATH "${__ret__}")
    ENDIF (NOT WIN32)
ENDIF (NOT DEFINED NATIVE_INCLUDE_PATH)

IF (DEFINED NATIVE_INCLUDE_PATH)
    SET(COMMON_CXXFLAGS "-DNATIVE_INCLUDE_PATH=\"${NATIVE_INCLUDE_PATH}\"")
ENDIF (DEFINED NATIVE_INCLUDE_PATH)

DEBUGMESSAGE(1 "GCC_FULLLIBDIR[${GCC_FULLLIBDIR}]")

IF (GCC_FULLLIBDIR)
    IF (EXISTS "${GCC_FULLLIBDIR}/libsupc++.a")
#       SET(ST_OBJADDE -Wl,-Bstatic)
        SET_APPEND(ST_OBJADDE ${GCC_FULLLIBDIR}/libsupc++.a)
        SET_APPEND(ST_OBJADDE ${GCC_FULLLIBGCCDIR}/libgcc.a)

        IF (EXISTS ${GCC_FULLLIBGCCDIR}/libgcc_eh.a)
            SET_APPEND(ST_OBJADDE ${GCC_FULLLIBGCCDIR}/libgcc_eh.a)
        ENDIF (EXISTS ${GCC_FULLLIBGCCDIR}/libgcc_eh.a)

        IF (ARCOS)
            SET_APPEND(ST_OBJADDE -lc_kt)
        ELSE (ARCOS)
            IF ("${CMAKE_BUILD_TYPE}" STREQUAL "profile" OR "${CMAKE_BUILD_TYPE}" STREQUAL "Profile" )
                SET_APPEND(ST_OBJADDE -lc_p)
            ELSE ("${CMAKE_BUILD_TYPE}" STREQUAL "profile" OR "${CMAKE_BUILD_TYPE}" STREQUAL "Profile" )
                SET_APPEND(ST_OBJADDE -lc)
            ENDIF ("${CMAKE_BUILD_TYPE}" STREQUAL "profile" OR "${CMAKE_BUILD_TYPE}" STREQUAL "Profile" )
        ENDIF (ARCOS)

        SET_APPEND(ST_OBJADDE
            -lm ${THREADLIB}
        )

        IF (EXISTS ${GCC_FULLCRTOBJDIR}/crtend.o)
            SET_APPEND(ST_OBJADDE ${GCC_FULLCRTOBJDIR}/crtend.o)
        ENDIF (EXISTS ${GCC_FULLCRTOBJDIR}/crtend.o)

    #   SET_APPEND(ST_OBJADDE -Wl,-Bdynamic)
        IF ("${CMAKE_BUILD_TYPE}" STREQUAL "profile" OR "${CMAKE_BUILD_TYPE}" STREQUAL "Profile" )
            SET(CRT1_NAME gcrt1.o)
        ELSE ("${CMAKE_BUILD_TYPE}" STREQUAL "profile" OR "${CMAKE_BUILD_TYPE}" STREQUAL "Profile" )
            SET(CRT1_NAME crt1.o)
        ENDIF ("${CMAKE_BUILD_TYPE}" STREQUAL "profile" OR "${CMAKE_BUILD_TYPE}" STREQUAL "Profile" )

        SET_IF_NOTSET(CRT1_PATH /usr/lib)
        IF (NOT EXISTS ${CRT1_PATH}/${CRT1_NAME} AND "${HARDWARE_TYPE}" MATCHES "x86_64")
            SET(CRT1_PATH /usr/lib64)
        ENDIF (NOT EXISTS ${CRT1_PATH}/${CRT1_NAME} AND "${HARDWARE_TYPE}" MATCHES "x86_64")
        IF (NOT EXISTS ${CRT1_PATH}/${CRT1_NAME} AND "${HARDWARE_TYPE}" MATCHES "i686")
            SET(CRT1_PATH /usr/lib/i386-linux-gnu)
        ENDIF (NOT EXISTS ${CRT1_PATH}/${CRT1_NAME} AND "${HARDWARE_TYPE}" MATCHES "i686")
        IF (NOT EXISTS ${CRT1_PATH}/${CRT1_NAME} AND "${HARDWARE_TYPE}" MATCHES "x86_64")
            SET(CRT1_PATH /usr/lib/x86_64-linux-gnu)
        ENDIF (NOT EXISTS ${CRT1_PATH}/${CRT1_NAME} AND "${HARDWARE_TYPE}" MATCHES "x86_64")

        IF (NOT EXISTS ${CRT1_PATH}/${CRT1_NAME})
            DEBUGMESSAGE(SEND_ERROR "cannot find ${CRT1_NAME} at /usr/lib and /usr/lib64 and ${CRT1_PATH}. Set CRT1_PATH in your local configuration. HARDWARE_TYPE: ${HARDWARE_TYPE}")
        ENDIF (NOT EXISTS ${CRT1_PATH}/${CRT1_NAME})

        IF (EXISTS ${CRT1_PATH}/crtn.o)
            SET_APPEND(ST_OBJADDE ${CRT1_PATH}/crtn.o)
        ENDIF (EXISTS ${CRT1_PATH}/crtn.o)

        SET(ST_LDFLAGS
            -nostdlib
            ${CRT1_PATH}/${CRT1_NAME}
        )

        IF (EXISTS ${CRT1_PATH}/crti.o)
            SET_APPEND(ST_LDFLAGS ${CRT1_PATH}/crti.o)
        ENDIF (EXISTS ${CRT1_PATH}/crti.o)

        IF (EXISTS ${GCC_FULLCRTOBJDIR}/crtbegin.o)
            SET_APPEND(ST_LDFLAGS ${GCC_FULLCRTOBJDIR}/crtbegin.o)
        ENDIF (EXISTS ${GCC_FULLCRTOBJDIR}/crtbegin.o)

        IF (EXISTS ${GCC_FULLCRTOBJDIR}/crt3.o)
            SET_APPEND(ST_LDFLAGS ${GCC_FULLCRTOBJDIR}/crt3.o)
        ENDIF (EXISTS ${GCC_FULLCRTOBJDIR}/crt3.o)
    ELSE (EXISTS "${GCC_FULLLIBDIR}/libsupc++.a")
        IF (USE_STATIC_CPP_RUNTIME)
            MESSAGE(SEND_ERROR "tools.cmake: ${GCC_FULLLIBDIR}/libsupc++.a doesn't exist (with positive USE_STATIC_CPP_RUNTIME)")
        ENDIF (USE_STATIC_CPP_RUNTIME)
    ENDIF (EXISTS "${GCC_FULLLIBDIR}/libsupc++.a")
ELSE (GCC_FULLLIBDIR)
    IF (NOT WIN32)
        IF (USE_STATIC_CPP_RUNTIME)
            MESSAGE(SEND_ERROR "tools.cmake: Can't get GCC_FULLLIBDIR (with positive USE_STATIC_CPP_RUNTIME)")
        ENDIF (USE_STATIC_CPP_RUNTIME)
    ENDIF (NOT WIN32)
ENDIF (GCC_FULLLIBDIR)

SEPARATE_ARGUMENTS_SPACE(ST_LDFLAGS)
SEPARATE_ARGUMENTS_SPACE(ST_OBJADDE)
#
# End of USE_STATIC_CPP_RUNTIME)
#

SET_IF_NOTSET(AWK       awk)
SET_IF_NOTSET(TAIL      tail)
SET_IF_NOTSET(ECHODIR       echo)
SET_IF_NOTSET(ECHO      echo)

IF (NOT "5" GREATER "0${FREEBSD_VER}")
    SET_IF_NOTSET(THREADLIB     -lthr)
ELSEIF (LINUX)
    SET_IF_NOTSET(THREADLIB     -lpthread)
ELSEIF (WIN32)
    SET(THREADLIB)
ELSE (NOT "5" GREATER "0${FREEBSD_VER}")
    SET_IF_NOTSET(THREADLIB     -pthread)
ENDIF (NOT "5" GREATER "0${FREEBSD_VER}")

SET_IF_NOTSET(TOUCH     touch)
SET_IF_NOTSET(DBC2CPP       "echo --- Define dependency to dbc2cpp explicitly in your project's CMakeLists.txt ---")
#SET_IF_NOTSET(DBC2CPP      ${ARCADIA_BUILD_ROOT}/yweb/webutil/dbc2cpp/dbc2cpp) # used only in check/mk_cvs_tag.sh

SET_IF_NOTSET(INSTALLFLAGS  "-c -m 755")
SET_IF_NOTSET(INSTFLAGS_755 "-c -m 755")
SET_IF_NOTSET(INSTFLAGS_644 "-c -m 644")



#PEERDIR    ?=  # peer dir for current

IF (NOT IGNORE_INVALID_TOOLS AND INVALID_TOOLS)
    MESSAGE(FATAL_ERROR "tools.cmake, fatal: one or more tools are missing or have invalid version (${INVALID_TOOLS}). Build failed")
ENDIF (NOT IGNORE_INVALID_TOOLS AND INVALID_TOOLS)

