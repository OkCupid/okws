# Copyright OkCupid 2016

include(${OKWS_SOURCE_DIR}/CMakeHelpers.cmake)

set(SRC pub3astdumper.C)

set(TAMED_SRC cache.T
              pub3stage1.T
              pub3.T
              pub3trace_replayer.T
              pubd.T
	      pubserv.T)

set(HEADERS "")

PreprocessTamedFiles(SRC HEADERS "${TAMED_SRC}")
add_custom_target(pub_headers DEPENDS ${HEADERS})

include_directories(${GLOBAL_INCLUDES}
                    ${OKWS_SOURCE_DIR}/libpub/
                    ${OKWS_BINARY_DIR}/libpub/
                    ${OKWS_SOURCE_DIR}/librfn/
		    ${CMAKE_CURRENT_SOURCE_DIR}/
		    ${CMAKE_CURRENT_BINARY_DIR}/)

link_directories(${GLOBAL_LINKS} /opt/stmd/lib/)

set(LINK_LIBS libpub aapp tame sfsmisc sfscrypt arpc safeptr async gmp dl z resolv snappy hiredis
    stdc++ m nsl pcre pthread librfn libahttp)

#add_executable(pub3trace_replayer ${CMAKE_CURRENT_BINARY_DIR}/pub3trace_replayer.cxx)
#target_link_libraries(pub3trace_replayer PUBLIC ${LINK_LIBS})
#
#add_executable(pub3stage1 ${CMAKE_CURRENT_BINARY_DIR}/pub3stage1.cxx)
#target_link_libraries(pub3stage1 PUBLIC ${LINK_LIBS})

#add_executable(pub3astdumper ${CMAKE_CURRENT_SOURCE_DIR}/pub3astdumper.C)
#target_link_libraries(pub3astdumper PUBLIC ${LINK_LIBS})

add_executable(pubd ${CMAKE_CURRENT_BINARY_DIR}/pubd.cxx
                    ${CMAKE_CURRENT_BINARY_DIR}/pubserv.cxx
                    ${CMAKE_CURRENT_BINARY_DIR}/cache.cxx)
target_link_libraries(pubd PUBLIC ${LINK_LIBS})
add_dependencies(pubd librfn)
