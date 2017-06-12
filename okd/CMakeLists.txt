# Copyright OkCupid 2016

include(${CMAKE_SOURCE_DIR}/CMakeHelpers.cmake)

set(SRC mgrsrv.C
	okld_script.C)

set(TAMED_SRC child.T
	      okd.T
	      okldch.T
	      okld.T
	      okmgr.T
	      shutdown.T
	      stats.T)

set(HEADERS "")

PreprocessTamedFiles(SRC HEADERS "${TAMED_SRC}")
add_custom_target(okd_headers DEPENDS ${HEADERS})

include_directories(${GLOBAL_INCLUDES}
                    /usr/include/mysql/
                    ${OKWS_SOURCE_DIR}/libaok/
                    ${OKWS_BINARY_DIR}/libaok/
                    ${OKWS_SOURCE_DIR}/libpub/
                    ${OKWS_BINARY_DIR}/libpub/
                    ${OKWS_BINARY_DIR}/
                    ${OKWS_SOURCE_DIR}/libahttp/
                    ${OKWS_BINARY_DIR}/libahttp/
                    ${OKWS_SOURCE_DIR}/libokxml/
                    ${OKWS_BINARY_DIR}/libokxml/
		    ${CMAKE_CURRENT_SOURCE_DIR}/
		    ${CMAKE_CURRENT_BINARY_DIR}/)

link_directories(${GLOBAL_LINKS} /usr/include/mysql/ /opt/stmd/lib/)

set(LINK_LIBS libezdb libaok librfn libweb libokssl libokxml libahttp libpub aapp
        tame sfsmisc sfscrypt arpc safeptr async gmp dl
        expat ssl crypto z resolv snappy hiredis mysqlclient
        m nsl pcre pthread libamysql libamt_pthread libamt)


add_executable(okd ${CMAKE_CURRENT_BINARY_DIR}/okd.cxx
                   ${CMAKE_CURRENT_BINARY_DIR}/child.cxx
                   ${CMAKE_CURRENT_BINARY_DIR}/stats.cxx
                   ${CMAKE_CURRENT_BINARY_DIR}/shutdown.cxx)
target_link_libraries(okd PUBLIC ${LINK_LIBS})

add_executable(okld ${CMAKE_CURRENT_BINARY_DIR}/okld.cxx
                    ${CMAKE_CURRENT_BINARY_DIR}/okldch.cxx
                    ${CMAKE_CURRENT_SOURCE_DIR}/okld_script.C)
target_link_libraries(okld PUBLIC ${LINK_LIBS})
