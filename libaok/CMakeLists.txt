# Copyright OkCupid 2016

include(${OKWS_SOURCE_DIR}/CMakeHelpers.cmake)

set(SRC direct.C
	locale.C
	ok3adapter.C
	okprotutil.C
	xdrsnappy.C)

set(TAMED_SRC log.T
	      ok3.T
	      ok.T
	      xmlrpcsrv.T
	      ok3adapter.Th
	      xmlrpcsrv.Th)

set(XML_PROT_FILES okprotext.x)
set(PROT_FILES okprot.x)

set(HEADERS "")

PreprocessTamedFiles(SRC HEADERS "${TAMED_SRC}")
PreprocessXmlProtFiles(LIBPROT_XML_C_SRC LIBPROT_XML_H_SRC "${XML_PROT_FILES}")
OkwsPreprocessXmlProtFiles(LIBPROT_C_SRC LIBPROT_H_SRC "${PROT_FILES}")
add_custom_target(libaok_headers DEPENDS ${HEADERS} ${LIBPROT_H_SRC} ${LIBPROT_XML_H_SRC})

include_directories(${GLOBAL_INCLUDES}
                    ${OKWS_BINARY_DIR}/
                    ${OKWS_SOURCE_DIR}/librfn/
                    ${OKWS_BINARY_DIR}/libpub/
                    ${OKWS_SOURCE_DIR}/pub/
                    ${OKWS_BINARY_DIR}/pub/
                    ${OKWS_SOURCE_DIR}/libpub/
                    ${OKWS_SOURCE_DIR}/libokxml/
                    ${OKWS_SOURCE_DIR}/libahttp/
                    ${CMAKE_CURRENT_SOURCE_DIR}/
		    ${CMAKE_CURRENT_BINARY_DIR}/)

if(STATIC)
    add_library(libaok STATIC ${SRC} ${LIBPROT_XML_C_SRC} ${LIBPROT_C_SRC})
    add_dependencies(libaok libpub_headers libaok_headers)
    target_link_libraries(libaok PUBLIC snappy libahttp libamt_pthread libamt libokssl libokxml libpub libredis librfn libweb)
else()
    add_library(libaok SHARED ${SRC} ${LIBPROT_XML_C_SRC} ${LIBPROT_C_SRC})
    add_dependencies(libaok libaok_headers)
endif()
