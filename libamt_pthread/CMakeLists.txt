# Copyright OkCupid 2016

include(${CMAKE_SOURCE_DIR}/CMakeHelpers.cmake)

set(SRC mtdispatch_pthread.C)

set(TAMED_SRC "")

set(HEADERS "")

PreprocessTamedFiles(SRC HEADERS "${TAMED_SRC}")
add_custom_target(libamt_pthread_headers DEPENDS ${HEADERS})

include_directories(${GLOBAL_INCLUDES}
                    ${OKWS_SOURCE_DIR}/
                    ${OKWS_SOURCE_DIR}/libamt/
                    ${OKWS_BINARY_DIR}/libamt/
                    ${OKWS_SOURCE_DIR}/libpub/
                    ${OKWS_BINARY_DIR}/libpub/
		    ${CMAKE_CURRENT_SOURCE_DIR}/
		    ${CMAKE_CURRENT_BINARY_DIR}/)

add_library(libamt_pthread SHARED ${SRC})
