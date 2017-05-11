# Copyright OkCupid 2016

include(${OKWS_SOURCE_DIR}/CMakeHelpers.cmake)

set(SRC abuf.C
	abuf_pipe.C
	ahttp.C
	ahutil.C
	aparse.C
	cgi.C
	err.C
	hdr.C
	httpconst.C
	inhdr.C
	kmp.C
	mimetypes.C
	mpfd.C
	pair.C
	scratch.C
	suiolite.C)

set(TAMED_SRC ahparse.T
	      ahttp2.T
	      resp2.T
	      resp.T)

set(HEADERS "")

PreprocessTamedFiles(SRC HEADERS "${TAMED_SRC}")
add_custom_target(libahttp_headers DEPENDS ${HEADERS})

include_directories(${GLOBAL_INCLUDES}
                    ${OKWS_SOURCE_DIR}/libpub/
                    ${OKWS_BINARY_DIR}/libpub/
		    ${CMAKE_CURRENT_SOURCE_DIR}/
		    ${CMAKE_CURRENT_BINARY_DIR}/)
if(STATIC)
    add_library(libahttp STATIC ${SRC})
    add_dependencies(libahttp libpub_headers pub_headers)
    target_link_libraries(libahttp PUBLIC libamt libamt_pthread libamysql libaok libokssl libokxml libpub libredis librfn libweb)
else()
    add_library(libahttp SHARED ${SRC})
    add_dependencies(libahttp libpub_headers pub_headers)
endif()
