cmake_minimum_required (VERSION 3.6.2)
project (OKWS)

FIND_PACKAGE(BISON REQUIRED)
FIND_PACKAGE(FLEX REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
set(OKWS_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(OKWS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# in source code replacements
add_definitions(-DOKWS_CONFIG_DIR="\\"/usr/local/etc/okws\\"")
add_definitions(-DOKWS_VERSION="\\"3.2.0\\"")
add_definitions(-DVERSION="\\"3.2.0\\"")
set(okws_version_major 3)
set(okws_version_minor 2)
set(okws_version_patchlevel 0)

# TODO don't do this
add_definitions(-DPATH_CPP="/usr/bin/cpp")

include(CheckSymbolExists)
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckLibraryExists)

set(GLOBAL_INCLUDES
    /usr/include/
    /usr/include/mysql/)

set(GLOBAL_LINKS /usr/lib/ # ld.gold doesn't look here by default
                 /usr/local/lib/) # ld.gold doesn't look here by default

find_package(OpenSSL)

if(Static)
    set(GLOBAL_LINKS ${GLOBAL_LINKS} /usr/local/lib/sfslite-1.2/hiperf/)
    set(GLOBAL_INCLUDES ${GLOBAL_INCLUDES} /usr/local/include/sfslite-1.2/hiperf/)
else()
    set(GLOBAL_LINKS ${GLOBAL_LINKS} /usr/local/lib/sfslite-1.2/shopt/)
    set(GLOBAL_INCLUDES ${GLOBAL_INCLUDES} /usr/local/include/sfslite-1.2/shopt/)
endif()


set(CMAKE_REQUIRED_INCLUDES /usr/include/mysql)
check_include_files(mysql.h HAVE_MYSQL_H)
if(HAVE_MYSQL_H)
    set(HAVE_MYSQL True)
    set(HAVE_MYSQL_BIND True)
    set(HAVE_MYSQL_BINDFUNCS True)
    # TODO fix these checks
    #CHECK_LIBRARY_EXISTS(mysqlclient mysql_real_connect mysql.h HAVE_MYSQL)
    #CHECK_LIBRARY_EXISTS(mysqlclient mysql_stmt_bind_param mysql.h HAVE_MYSQL_BIND)
    #CHECK_LIBRARY_EXISTS(mysqlclient mysql_stmt_bind_param mysql.h HAVE_MYSQL_BINDFUNCS)
    #CHECK_SYMBOL_EXISTS (mysql_stmt_bind_param /usr/include/mysql/mysql.h HAVE_MYSQL_BINDFUNCS)
endif()

CHECK_INCLUDE_FILES (malloc.h HAVE_MALLOC_H)
CHECK_INCLUDE_FILES (memory.h HAVE_MEMORY_H)
CHECK_INCLUDE_FILES (sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILES (sys/stat.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILES (stdlib.h STDC_HEADERS)
CHECK_INCLUDE_FILES (strings.h HAVE_STRING_H)
CHECK_INCLUDE_FILES (inttypes.h HAVE_INTTYPES_H)
CHECK_INCLUDE_FILES (stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILES (unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILES (expat.h HAVE_EXPAT)
CHECK_INCLUDE_FILES (pthread.h HAVE_PTHREADS)
CHECK_INCLUDE_FILES ("sys/param.h;sys/mount.h" HAVE_SYS_MOUNT_H)
CHECK_SYMBOL_EXISTS (mpz_xor gmp.h HAVE_GMP_CXX_OPS)
CHECK_INCLUDE_FILES (openssl/ssl.h HAVE_SSL)
CHECK_INCLUDE_FILES (/usr/include/mysql/mysql.h HAVE_MYSQL)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/okwsconf.h.in ${CMAKE_CURRENT_BINARY_DIR}/okwsconf.h)

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
add_subdirectory(ssld)
add_subdirectory(logd)
