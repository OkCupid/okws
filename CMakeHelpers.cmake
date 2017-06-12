# Ben Brittain
# Copyright OkCupid 2016
# Moving the Build System into 2012

set(OKWS_XMLRPCC ${OKWS_BINARY_DIR}/xmlrpcc/xmlrpcc)

FUNCTION(PreprocessTamedFiles RET RET_HEADERS SOURCES)
    set(OKWS_TAME /usr/local/lib/sfslite-1.2/shopt/tame)
    if(STATIC)
        set(OKWS_TAME /usr/local/lib/sfslite-1.2/hiperf/tame)
    endif()
    set(_result ${${RET}})
    set(_result_headers ${${RET_HEADERS}})
    foreach(t_file ${SOURCES})
        get_filename_component(name_file ${t_file} NAME)
        if ("${t_file}" MATCHES ".Th$")
            string(REPLACE ".Th" ".h" cxx_file ${CMAKE_CURRENT_BINARY_DIR}/${t_file})
        else()
            string(REPLACE ".T" ".cxx" cxx_file ${CMAKE_CURRENT_BINARY_DIR}/${t_file})
        endif()
        add_custom_command(
            OUTPUT ${cxx_file}
            COMMAND ${OKWS_TAME}
            ARGS    ${CMAKE_CURRENT_SOURCE_DIR}/${t_file} > ${cxx_file}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${t_file} ${OKWS_TAME})
        if ("${t_file}" MATCHES ".Th$")
            list(APPEND _result_headers ${cxx_file})
        else()
            list(APPEND _result ${cxx_file})
        endif()
        endforeach(t_file ${SOURCES})
    SET(${RET} ${_result} PARENT_SCOPE)
    SET(${RET_HEADERS} ${_result_headers} PARENT_SCOPE)
ENDFUNCTION()

FUNCTION(OkwsPreprocessProtFiles CFILES HFILES SOURCES)
    set(OKWS_RPCC /usr/local/lib/sfslite-1.2/shopt/rpcc)
    if(STATIC)
        set(OKWS_RPCC /usr/local/lib/sfslite-1.2/hiperf/rpcc)
    endif()
    set(_c_result)
    set(_h_result)
    foreach(p_file ${SOURCES})
        get_filename_component(name_file ${p_file} NAME)
        if ("${p_file}" MATCHES ".v$") #hacky but people used the wrong suffixes a lot :(
            string(REPLACE ".v" ".h" h_file ${CMAKE_CURRENT_BINARY_DIR}/${p_file})
            string(REPLACE ".v" ".C" c_file ${CMAKE_CURRENT_BINARY_DIR}/${p_file})
        else()
            string(REPLACE ".x" ".h" h_file ${CMAKE_CURRENT_BINARY_DIR}/${p_file})
            string(REPLACE ".x" ".C" c_file ${CMAKE_CURRENT_BINARY_DIR}/${p_file})
        endif()
        add_custom_command(
            OUTPUT ${h_file}
            COMMAND ${OKWS_RPCC}
            ARGS    -X -o ${h_file} -h ${CMAKE_CURRENT_SOURCE_DIR}/${p_file}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${p_file} ${OKWS_RPCC})
        add_custom_command(
            OUTPUT ${c_file}
            COMMAND ${OKWS_RPCC}
            ARGS    -X -o ${c_file} -c ${CMAKE_CURRENT_SOURCE_DIR}/${p_file}
            DEPENDS ${h_file} ${OKWS_RPCC})
        list(APPEND _c_result ${c_file})
        list(APPEND _h_result ${h_file})
    endforeach(p_file ${SOURCES})
    SET(${HFILES} ${_h_result} PARENT_SCOPE)
    SET(${CFILES} ${_c_result} PARENT_SCOPE)
ENDFUNCTION()

FUNCTION(OkwsPreprocessXmlProtFiles CFILES HFILES SOURCES)
    set(OKWS_RPCC /usr/local/lib/sfslite-1.2/shopt/rpcc)
    if(STATIC)
        set(OKWS_RPCC /usr/local/lib/sfslite-1.2/hiperf/rpcc)
    endif()
    set(_c_result)
    set(_h_result)
    foreach(p_file ${SOURCES})
        get_filename_component(name_file ${p_file} NAME)
        if ("${p_file}" MATCHES ".v$") #hacky but people used the wrong suffixes a lot :(
            string(REPLACE ".v" ".h" h_file ${CMAKE_CURRENT_BINARY_DIR}/${p_file})
            string(REPLACE ".v" ".C" c_file ${CMAKE_CURRENT_BINARY_DIR}/${p_file})
        else()
            string(REPLACE ".x" ".h" h_file ${CMAKE_CURRENT_BINARY_DIR}/${p_file})
            string(REPLACE ".x" ".C" c_file ${CMAKE_CURRENT_BINARY_DIR}/${p_file})
        endif()
        add_custom_command(
            OUTPUT ${h_file}
            COMMAND ${OKWS_RPCC}
            ARGS    -o ${h_file} -h ${CMAKE_CURRENT_SOURCE_DIR}/${p_file}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${p_file} ${OKWS_RPCC})
        add_custom_command(
            OUTPUT ${c_file}
            COMMAND ${OKWS_RPCC}
            ARGS    -o ${c_file} -c ${CMAKE_CURRENT_SOURCE_DIR}/${p_file}
            DEPENDS ${h_file} ${OKWS_RPCC})
        list(APPEND _c_result ${c_file})
        list(APPEND _h_result ${h_file})
    endforeach(p_file ${SOURCES})
    SET(${HFILES} ${_h_result} PARENT_SCOPE)
    SET(${CFILES} ${_c_result} PARENT_SCOPE)
ENDFUNCTION()

FUNCTION(PreprocessXmlProtFiles CFILES HFILES SOURCES)
    set(_c_result)
    set(_h_result)
    foreach(p_file ${SOURCES})
        get_filename_component(name_file ${p_file} NAME)
        if ("${p_file}" MATCHES ".v$") #hacky but people used the wrong suffixes a lot :(
            string(REPLACE ".v" ".h" h_file ${CMAKE_CURRENT_BINARY_DIR}/${p_file})
            string(REPLACE ".v" ".C" c_file ${CMAKE_CURRENT_BINARY_DIR}/${p_file})
        else()
            string(REPLACE ".x" ".h" h_file ${CMAKE_CURRENT_BINARY_DIR}/${p_file})
            string(REPLACE ".x" ".C" c_file ${CMAKE_CURRENT_BINARY_DIR}/${p_file})
        endif()
        add_custom_command(
            OUTPUT ${h_file}
            COMMAND LD_LIBRARY_PATH=/usr/local/lib/sfslite-1.2/shopt/ ${OKWS_XMLRPCC}
            ARGS    -o ${h_file} -h ${CMAKE_CURRENT_SOURCE_DIR}/${p_file}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${p_file} ${OKWS_XMLRPCC})
        add_custom_command(
            OUTPUT ${c_file}
            COMMAND LD_LIBRARY_PATH=/usr/local/lib/sfslite-1.2/shopt/ ${OKWS_XMLRPCC}
            ARGS    -o ${c_file} -c ${CMAKE_CURRENT_SOURCE_DIR}/${p_file}
            DEPENDS ${h_file} ${OKWS_XMLRPCC})
        list(APPEND _c_result ${c_file})
        list(APPEND _h_result ${h_file})
    endforeach(p_file ${SOURCES})
    SET(${HFILES} ${_h_result} PARENT_SCOPE)
    SET(${CFILES} ${_c_result} PARENT_SCOPE)
ENDFUNCTION()
