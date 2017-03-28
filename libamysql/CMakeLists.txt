# Copyright OkCupid 2016

include(${CMAKE_SOURCE_DIR}/CMakeHelpers.cmake)

set(SRC amysql.C
	gmtoff.C
	mybind.C
	mystmt.C
	tz.C)

set(TAMED_SRC "")

set(HEADERS "")

PreprocessTamedFiles(SRC HEADERS "${TAMED_SRC}")

add_custom_command(
    OUTPUT mystmt_ag.h
    COMMAND perl
    ARGS ${CMAKE_CURRENT_SOURCE_DIR}/mystmt_ag.pl -h > ${CMAKE_CURRENT_BINARY_DIR}/mystmt_ag.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mystmt_ag.pl)

add_custom_command(
    OUTPUT mystmt_ag.C
    COMMAND perl
    ARGS ${CMAKE_CURRENT_SOURCE_DIR}/mystmt_ag.pl -c > ${CMAKE_CURRENT_BINARY_DIR}/mystmt_ag.C
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mystmt_ag.pl)

add_custom_target(libmystmt_headers DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/mystmt_ag.h ${HEADERS} ${LIBPROT_XML_H_SRC})

include_directories(${GLOBAL_INCLUDES}
                    /usr/include/mysql/
                    ${OKWS_BINARY_DIR}/
                    ${OKWS_SOURCE_DIR}/libpub/
                    ${OKWS_BINARY_DIR}/libpub/
                    ${OKWS_SOURCE_DIR}/libweb/
                    ${OKWS_BINARY_DIR}/libweb/
                    ${OKWS_SOURCE_DIR}/libokxml/
                    ${OKWS_SOURCE_DIR}/libamt/
		    ${CMAKE_CURRENT_SOURCE_DIR}/
		    ${CMAKE_CURRENT_BINARY_DIR}/)

add_custom_target(libamysql_headers DEPENDS ${HEADERS})

if(STATIC)
    add_library(libamysql STATIC ${SRC} ${CMAKE_CURRENT_BINARY_DIR}/mystmt_ag.C)
    add_dependencies(libamysql libweb_headers)
    target_link_libraries(libamysql PUBLIC libahttp libamt_pthread libamt libaok libokssl libokxml libpub libredis librfn libweb mysqlclient)
else()
    add_library(libamysql SHARED ${SRC} ${CMAKE_CURRENT_BINARY_DIR}/mystmt_ag.C)
    add_dependencies(libamysql libweb_headers)
    target_link_libraries(libamysql mysqlclient)
endif()
