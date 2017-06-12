# Copyright OkCupid 2016

include(${OKWS_SOURCE_DIR}/CMakeHelpers.cmake)

set(SRC gencfile.C
	genheader.C
	rpcc.C)

set(TAMED_SRC "")

set(HEADERS "")

PreprocessTamedFiles(SRC HEADERS "${TAMED_SRC}")

bison_target(parser parse.yy ${CMAKE_CURRENT_BINARY_DIR}/parse.cc)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/parse.h
    COMMAND cp
    ARGS    ${BISON_parser_OUTPUT_HEADER} ${CMAKE_CURRENT_BINARY_DIR}/parse.h
    DEPENDS ${BISON_parser_OUTPUT_HEADER})
add_custom_target(xmlrpcc_headers DEPENDS ${HEADERS} ${CMAKE_CURRENT_BINARY_DIR}/parse.h)
flex_target(scanner scan.ll ${CMAKE_CURRENT_BINARY_DIR}/scan.cc)

link_directories(${GLOBAL_LINKS})
include_directories(${GLOBAL_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/ ${CMAKE_CURRENT_BINARY_DIR}/)

add_executable(xmlrpcc ${CMAKE_CURRENT_SOURCE_DIR}/rpcc.C
                       ${CMAKE_CURRENT_SOURCE_DIR}/genheader.C
                       ${CMAKE_CURRENT_SOURCE_DIR}/gencfile.C
                       ${CMAKE_CURRENT_BINARY_DIR}/scan.cc
                       ${CMAKE_CURRENT_BINARY_DIR}/parse.cc)

target_link_libraries(xmlrpcc PUBLIC async dl pcre Threads::Threads resolv)
add_dependencies(xmlrpcc xmlrpcc_headers)
