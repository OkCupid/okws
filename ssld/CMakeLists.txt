# Copyright OkCupid 2016

include(${CMAKE_SOURCE_DIR}/CMakeHelpers.cmake)

set(SRC "")

set(TAMED_SRC okssld.T)

set(HEADERS "")

PreprocessTamedFiles(SRC HEADERS "${TAMED_SRC}")
add_custom_target(ssld_headers DEPENDS ${HEADERS})

include_directories(${GLOBAL_INCLUDES}
                    ${OKWS_SOURCE_DIR}/libokssl/
                    ${OKWS_BINARY_DIR}/libokssl/
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

link_directories(${GLOBAL_LINKS} /usr/include/mysql/)

set(LINK_LIBS libezdb libaok librfn libweb libokssl libokxml libahttp libpub aapp
        tame sfsmisc sfscrypt arpc safeptr async gmp dl
        expat ssl okcrypt z resolv snappy hiredis mysqlclient
        m nsl pcre pthread libamysql libamt_pthread libamt)

add_executable(okssld ${CMAKE_CURRENT_BINARY_DIR}/okssld.cxx)
target_link_libraries(okssld PUBLIC ${LINK_LIBS} OpenSSL::SSL)
