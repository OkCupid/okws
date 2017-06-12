# Copyright OkCupid 2016

include(${OKWS_SOURCE_DIR}/CMakeHelpers.cmake)

set(SRC debug.C
	escape.C
	jail.C
	json_rpc.C
	okconst.C
	precycle.C
	pub3base.C
	pub3cache.C
	pub3debug.C
	pub3env.C
	pub3expr.C
	pub3file.C
	pub3json2xdr.C
	pub3msgpack.C
	pub3obj.C
	pub3parse.C
	pub3profiler.C
	pub3ref.C
	pub3xdr.C
	pubutil.C
	rxx.C
	scalar.C
	slave.C
	stats.C
	zstr.C)

set(TAMED_SRC adebug.T
	      clone.T
	      pub3ast.T
	      pub3eng.T
	      pub3eval.T
	      pub3expub.T
	      pub3func.T
	      pub3heredoc.T
	      pub3hilev.T
	      pub3lib.T
	      pub3msgpackrpc.T
	      pub3out.T
	      pub3tracer.T
	      sync.T)

set(XML_PROT_FILES pub3prot.x)

set(HEADERS "")

PreprocessTamedFiles(SRC HEADERS "${TAMED_SRC}")
OkwsPreprocessXmlProtFiles(LIBPROT_XML_C_SRC LIBPROT_XML_H_SRC "${XML_PROT_FILES}")

bison_target(Parser parse.yy ${CMAKE_CURRENT_BINARY_DIR}/parser.cc)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/parse.h
    COMMAND cp
    ARGS    ${BISON_Parser_OUTPUT_HEADER} ${CMAKE_CURRENT_BINARY_DIR}/parse.h
    DEPENDS ${BISON_Parser_OUTPUT_HEADER})

flex_target(Scanner scan.ll ${CMAKE_CURRENT_BINARY_DIR}/scan.cc)
add_flex_bison_dependency(Scanner Parser)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/okws_sfs.h.in ${CMAKE_CURRENT_BINARY_DIR}/okws_sfs.h)

add_custom_target(libpub_headers DEPENDS
    ${HEADERS}
    ${CMAKE_CURRENT_BINARY_DIR}/okws_sfs.h
    ${LIBPROT_XML_H_SRC}
    ${CMAKE_CURRENT_BINARY_DIR}/parse.h)

include_directories(${GLOBAL_INCLUDES}
                    ${OKWS_SOURCE_DIR}/libokxml/
                    ${OKWS_BINARY_DIR}/libokxml/
		    ${CMAKE_CURRENT_SOURCE_DIR}/
		    ${CMAKE_CURRENT_BINARY_DIR}/)

if(STATIC)
    add_library(libpub STATIC ${SRC} ${LIBPROT_XML_C_SRC} ${BISON_Parser_OUTPUTS} ${FLEX_Scanner_OUTPUTS})
    add_dependencies(libpub libpub_headers)
    target_link_libraries(libpub PUBLIC libweb crypt sfsmisc async dl crypt dl crypto dl arpc z sfsmisc libahttp
                                        libamt_pthread libamysql libaok libokssl libokxml libredis librfn
                                        libweb pcre pcrecpp dl)
else()
    add_library(libpub SHARED ${SRC} ${LIBPROT_XML_C_SRC} ${BISON_Parser_OUTPUTS} ${FLEX_Scanner_OUTPUTS})
    add_dependencies(libpub libpub_headers)
endif()
