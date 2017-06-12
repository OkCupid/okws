# Copyright OkCupid 2016

include(${OKWS_SOURCE_DIR}/CMakeHelpers.cmake)

set(SRC email.C
	inputcheck.C
	okwc.C
	web.C)

set(TAMED_SRC okwc3.T
	      okwc4.T
	      okwcxml.T)

set(XML_PROT_FILES web_prot.x)

set(HEADERS "")

PreprocessTamedFiles(SRC HEADERS "${TAMED_SRC}")
PreprocessXmlProtFiles(LIBPROT_XML_C_SRC LIBPROT_XML_H_SRC "${XML_PROT_FILES}")
add_custom_target(libweb_headers DEPENDS ${HEADERS} ${LIBPROT_XML_H_SRC})

include_directories(${GLOBAL_INCLUDES}
                    ${OKWS_BINARY_DIR}/
                    ${OKWS_SOURCE_DIR}/libpub/
                    ${OKWS_BINARY_DIR}/libpub/
                    ${OKWS_SOURCE_DIR}/libahttp/
                    ${OKWS_SOURCE_DIR}/libokssl/
                    ${OKWS_SOURCE_DIR}/libokxml/
                    ${OKWS_SOURCE_DIR}/libaok/
		    ${CMAKE_CURRENT_SOURCE_DIR}/
		    ${CMAKE_CURRENT_BINARY_DIR}/)

if(STATIC)
    add_library(libweb STATIC ${SRC} ${LIBPROT_XML_C_SRC})
    add_dependencies(libweb libweb_headers)
    target_link_libraries(libweb PUBLIC snappy libahttp libamt_pthread libamysql libaok libokssl libokxml libpub libredis librfn)
else()
    add_library(libweb SHARED ${SRC} ${LIBPROT_XML_C_SRC})
    add_dependencies(libweb libweb_headers)
endif()
