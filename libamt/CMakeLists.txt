# Copyright OkCupid 2016

include(${CMAKE_SOURCE_DIR}/CMakeHelpers.cmake)

set(SRC mtdispatch.C)

set(TAMED_SRC "")

set(HEADERS "")

PreprocessTamedFiles(SRC HEADERS "${TAMED_SRC}")
add_custom_target(libamt_headers DEPENDS ${HEADERS})

include_directories(${GLOBAL_INCLUDES}
                    ${OKWS_SOURCE_DIR}/
                    ${OKWS_SOURCE_DIR}/libpub/
                    ${OKWS_BINARY_DIR}/libpub/
                    ${OKWS_SOURCE_DIR}/libamt_pthread/
                    ${OKWS_BINARY_DIR}/libamt_pthread/
		    ${CMAKE_CURRENT_SOURCE_DIR}/
		    ${CMAKE_CURRENT_BINARY_DIR}/)

add_library(libamt SHARED ${SRC})
