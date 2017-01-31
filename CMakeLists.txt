cmake_minimum_required (VERSION 2.8.11)
project (OKWS)

FIND_PACKAGE(BISON REQUIRED)
FIND_PACKAGE(FLEX REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
set(OKWS_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(OKWS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLINUX=1") # one of ours
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wextra -Wall -Werror -Wno-unused-parameter")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-ignored-qualifiers -Wno-missing-field-initializers")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qunused-arguments -Wno-error=deprecated-declarations ")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=unused-function -Wno-mismatched-tags")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-overloaded-virtual -Wno-predefined-identifier-outside-function")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-private-field -Wno-deprecated")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
#
#
## Debug build (Default)
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fcolor-diagnostics")
#
## Optimized build
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Ofast -fno-omit-frame-pointer")
#
set(GLOBAL_INCLUDES #todo fix this
#    ${CMAKE_BINARY_DIR}/libprot/ # here for convienence
#    ${CMAKE_BINARY_DIR}/money/libprot/ # here for convienence
#    ${CMAKE_BINARY_DIR}/shared/libprot/ # here for convienence
#    /usr/include/mysql/ # ${MYSQL_INCLUDES}
#    /usr/local/dsdc/0.26/shdbg/include/
    /usr/local/include/sfslite-1.2/shdbg/)
#    /usr/local/include/okws-3.1/shdbg/)

set(GLOBAL_LINKS /usr/lib/ # ld.gold doesn't look here by default
                 /usr/local/lib/) # ld.gold doesn't look here by default

# in source code replacements
add_definitions(-DOKWS_CONFIG_DIR="\\"/usr/local/etc/okws\\"")
add_definitions(-DOKWS_VERSION="\\"3.2.0\\"")
add_definitions(-DVERSION="\\"3.2.0\\"")
set(okws_version_major 3)
set(okws_version_minor 2)
set(okws_version_patchlevel 0)


# TODO don't do this
add_definitions(-DPATH_CPP="/usr/bin/cpp")


# Generate a compile_comands.json
# set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")


#option(ENABLE_CCACHE "enable caching compiler output" ON)
# TODO flags to turn off DISTCC/CCACHE
#option(ENABLE_DISTCC "Use distcc to speed up compilation." OFF)

#find_package(PkgConfig REQUIRED)
#pkg_search_module(OPENSSL REQUIRED openssl)
#if( OPENSSL_FOUND )
#    include_directories(${OPENSSL_INCLUDE_DIRS})
#    message(STATUS "Using OpenSSL ${OPENSSL_VERSION}")
#else()
#    # Error
#endif()
#
## TODO make work
##find_program(GOLD_LINKER gold)
##if(GOLD_LINKER)
##    message(STATUS "using the gold linker ${GOLD_LINKER}")
##    set(CMAKE_LINKER "${GOLD_LINKER}")
##    set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS}") # -B${CMAKE_SOURCE_DIR}/tools/build_system")
##endif()
#
#find_library(SASS NAMES sass HINTS "/usr/local/lib")
##set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} /usr/local/lib/)
#
#
## enable ccache usage
#find_program(CCACHE_FOUND ccache)
#if(CCACHE_FOUND)
#    message(STATUS "Using ccache for building")
#    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
#    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
#endif(CCACHE_FOUND)
#
## todo switch to icecream
## enable distcc/ccache usage
## if(DISTCC_FOUND)
##     if(CCACHE_FOUND)
##         set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "CCACHE_PREFIX=distcc ccache")
##         message(STATUS "Enabling distcc builds in ccache")
##     else()
##         set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE distcc)
##         message(STATUS "Using distcc for building")
##     endif()
## endif()
#
## All the things to build
#
#add_subdirectory(libokdb)
#add_subdirectory(libokutil)
#add_subdirectory(libmembership)
#add_subdirectory(libcache)
#add_subdirectory(libokgrid)
#add_subdirectory(libpbr)
#add_subdirectory(libpubsub)
#add_subdirectory(libgmock)
#add_subdirectory(libbase)
#add_subdirectory(liboktidy)
#add_subdirectory(money)
#add_subdirectory(libcupid)
#add_subdirectory(libmodule)
#add_subdirectory(libtest)
#add_subdirectory(liboksoap)
#add_subdirectory(libprot)
#add_subdirectory(libconn)
#add_subdirectory(rpcsrv)
#add_subdirectory(dbprox)
#add_subdirectory(websrv)
#add_subdirectory(offline)
#
#add_subdirectory(dependencies)

add_subdirectory(xmlrpcc)
add_subdirectory(pub)
add_subdirectory(libokxml)
add_subdirectory(libamt_pthread)
add_subdirectory(libamt)
add_subdirectory(libamysql)
add_subdirectory(okd)
add_subdirectory(librfn)
add_subdirectory(libaok)
add_subdirectory(libokssl)
add_subdirectory(libahttp)
add_subdirectory(libpub)
add_subdirectory(libweb)
add_subdirectory(ezdb)
add_subdirectory(libredis)
