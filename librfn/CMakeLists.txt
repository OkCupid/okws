# Copyright OkCupid 2016

include(${OKWS_SOURCE_DIR}/CMakeHelpers.cmake)

set(SRC dict.C
	import.C
	math.C
	regex.C
	rfn3.C
	time.C)

set(TAMED_SRC file.T
	      filters.T
	      list.T
	      primitives.T
	      sync.T)

set(HEADERS "")

PreprocessTamedFiles(SRC HEADERS "${TAMED_SRC}")
add_custom_target(librfn_headers DEPENDS ${HEADERS})

include_directories(${GLOBAL_INCLUDES}
                    ${OKWS_SOURCE_DIR}/libahttp/
                    ${OKWS_SOURCE_DIR}/libpub/
                    ${OKWS_BINARY_DIR}/libpub/
                    ${OKWS_SOURCE_DIR}/pub/
                    ${OKWS_BINARY_DIR}/pub/
		    ${CMAKE_CURRENT_SOURCE_DIR}/
		    ${CMAKE_CURRENT_BINARY_DIR}/)

if(STATIC)
    add_library(librfn STATIC ${SRC})
    add_dependencies(librfn libpub_headers pub_headers)
    target_link_libraries(librfn PUBLIC libahttp libamt_pthread libamysql libaok libokssl libokxml libpub libredis libweb)
else()
    add_library(librfn SHARED ${SRC})
    add_dependencies(librfn libpub_headers pub_headers)
endif()
