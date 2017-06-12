# Copyright OkCupid 2016

include(${OKWS_SOURCE_DIR}/CMakeHelpers.cmake)

set(SRC "")

set(TAMED_SRC redis.T)

set(HEADERS "")

PreprocessTamedFiles(SRC HEADERS "${TAMED_SRC}")
add_custom_target(libredis_headers DEPENDS ${HEADERS})

include_directories(${GLOBAL_INCLUDES}
                    ${OKWS_SOURCE_DIR}/libpub/
                    ${OKWS_BINARY_DIR}/libpub/
		    ${CMAKE_CURRENT_SOURCE_DIR}/
		    ${CMAKE_CURRENT_BINARY_DIR}/)

if(STATIC)
    add_library(libredis STATIC ${SRC})
    target_link_libraries(libredis hiredis)
    add_dependencies(libredis libpub_headers)
else()
    add_library(libredis SHARED ${SRC})
    add_dependencies(libredis libpub_headers)
endif()
