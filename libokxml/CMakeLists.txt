# Copyright OkCupid 2016

include(${OKWS_SOURCE_DIR}/CMakeHelpers.cmake)

set(SRC xdr_as_json.C
	xml_ahparse.C
	xmldata.C
	xmlgeneric.C
	xmlobj.C
	xmlparse.C
	xmlxlate.C)

set(TAMED_SRC xmlxlatemgr.T)

set(HEADERS "")

PreprocessTamedFiles(SRC HEADERS "${TAMED_SRC}")

#set(CONF_HEADER )
#add_custom_command(
#    OUTPUT okwsconf.h
#    COMMAND sed
#    ARGS    -f ${CMAKE_SOURCE_DIR}/autoconf.sed ${CONF_HEADER} > okwsconf.h
#    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${t_file})

add_custom_target(libokxml_headers DEPENDS ${HEADERS})

include_directories(${GLOBAL_INCLUDES}
                    ${OKWS_BINARY_DIR}/libpub/
                    ${OKWS_SOURCE_DIR}/libpub/
                    ${OKWS_SOURCE_DIR}/libahttp/
                    ${OKWS_BINARY_DIR}/
		    ${CMAKE_CURRENT_SOURCE_DIR}/
		    ${CMAKE_CURRENT_BINARY_DIR}/)

if(STATIC)
    add_library(libokxml STATIC ${SRC})
    add_dependencies(libokxml libokxml_headers libpub_headers)
    target_link_libraries(libokxml xml2 expat libpub)
else()
    add_library(libokxml SHARED ${SRC})
    add_dependencies(libokxml libokxml_headers libpub_headers)
endif()
