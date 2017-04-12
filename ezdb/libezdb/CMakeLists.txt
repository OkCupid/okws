# Copyright OkCupid 2016

include(${OKWS_SOURCE_DIR}/CMakeHelpers.cmake)

set(SRC bind.C
	field.C)

set(XML_PROT_FILES ezdb_prot.x)

set(TAMED_SRC ezdb.T)
set(HEADERS "")
set(LIBPROT_XML_C_SRC "")
set(LIBPROT_XML_H_SRC "")

PreprocessXmlProtFiles(LIBPROT_XML_C_SRC LIBPROT_XML_H_SRC "${XML_PROT_FILES}")
PreprocessTamedFiles(SRC HEADERS "${TAMED_SRC}")

add_custom_command(
    OUTPUT ezdb_ag.h
    COMMAND perl
    ARGS ${CMAKE_CURRENT_SOURCE_DIR}/ezdb_ag.pl -h > ${CMAKE_CURRENT_BINARY_DIR}/ezdb_ag.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ezdb_ag.pl)

add_custom_command(
    OUTPUT ezdb_ag.C
    COMMAND perl
    ARGS ${CMAKE_CURRENT_SOURCE_DIR}/ezdb_ag.pl -c > ${CMAKE_CURRENT_BINARY_DIR}/ezdb_ag.C
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ezdb_ag.pl)

add_custom_target(libezdb_headers DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/ezdb_ag.h ${HEADERS} ${LIBPROT_XML_H_SRC})

include_directories(${GLOBAL_INCLUDES}
                    /usr/include/mysql/
                    ${OKWS_BINARY_DIR}/
                    ${OKWS_SOURCE_DIR}/libokxml/
                    ${OKWS_SOURCE_DIR}/libamt/
                    ${OKWS_SOURCE_DIR}/libaok/
                    ${OKWS_SOURCE_DIR}/libpub/
                    ${OKWS_BINARY_DIR}/libpub/
                    ${OKWS_SOURCE_DIR}/libweb/
                    ${OKWS_BINARY_DIR}/libweb/
                    ${OKWS_SOURCE_DIR}/libamysql/
                    ${CMAKE_CURRENT_SOURCE_DIR}/
                    ${CMAKE_CURRENT_BINARY_DIR}/)

if(STATIC)
    add_library(libezdb STATIC ${SRC} ${LIBPROT_XML_C_SRC} ${CMAKE_CURRENT_BINARY_DIR}/ezdb_ag.C)
    add_dependencies(libezdb libezdb_headers)
    target_link_libraries(libezdb mysqlclient)

    add_library(libezdbsrv STATIC serve.C)
    add_dependencies(libezdbsrv libezdb_headers)
    target_link_libraries(libezdbsrv mysqlclient)
else()
    add_library(libezdb SHARED ${SRC} ${LIBPROT_XML_C_SRC} ${CMAKE_CURRENT_BINARY_DIR}/ezdb_ag.C)
    add_dependencies(libezdb libezdb_headers)

    add_library(libezdbsrv SHARED serve.C)
    add_dependencies(libezdbsrv libezdb_headers)
endif()
