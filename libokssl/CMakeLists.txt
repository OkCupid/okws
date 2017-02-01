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

add_library(libokssl SHARED ${SRC})
