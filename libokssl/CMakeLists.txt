# Copyright OkCupid 2016

include(${OKWS_SOURCE_DIR}/CMakeHelpers.cmake)

set(SRC util.C)

set(TAMED_SRC proxy.T
	      sslcon.T)

set(HEADERS "")

PreprocessTamedFiles(SRC HEADERS "${TAMED_SRC}")
add_custom_target(libokssl_headers DEPENDS ${HEADERS})

include_directories(${GLOBAL_INCLUDES}
                    ${OKWS_BINARY_DIR}/
                    ${OKWS_SOURCE_DIR}/libpub/
                    ${OKWS_BINARY_DIR}/libpub/
                    ${OKWS_SOURCE_DIR}/libahttp/
		    ${CMAKE_CURRENT_SOURCE_DIR}/
		    ${CMAKE_CURRENT_BINARY_DIR}/)

if(STATIC)
    add_library(libokssl STATIC ${SRC})
    add_dependencies(libokssl libpub_headers)
    target_link_libraries(libokssl libpub ssl tame crypt crypto)
else()
    add_library(libokssl SHARED ${SRC})
    add_dependencies(libokssl libpub_headers)
endif()
