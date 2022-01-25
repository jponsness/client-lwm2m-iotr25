cmake_minimum_required (VERSION 3.13)

project (lwm2mclient C)

option(DTLS "Enable DTLS" OFF)

include(${CMAKE_CURRENT_LIST_DIR}/wakaama/core/wakaama.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/wakaama/coap/coap.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/wakaama/data/data.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/wakaama/examples/shared/shared.cmake)

add_compile_definitions(LWM2M_CLIENT_MODE)
add_compile_definitions(LWM2M_BOOTSTRAP)
add_compile_definitions(LWM2M_SUPPORT_JSON)

if(LWM2M_VERSION VERSION_GREATER "1.0")
    add_compile_definitions(LWM2M_SUPPORT_SENML_JSON)
else()
    add_compile_definitions(LWM2M_SUPPORT_JSON)
endif()

if(LWM2M_RAW_BLOCK1_REQUESTS)
    add_compile_definitions(LWM2M_RAW_BLOCK1_REQUESTS)
endif()

add_definitions(${SHARED_DEFINITIONS} ${WAKAAMA_DEFINITIONS})

include_directories (${WAKAAMA_HEADERS_DIR} ${COAP_HEADERS_DIR} ${DATA_HEADERS_DIR} ${WAKAAMA_SOURCES_DIR} ${SHARED_INCLUDE_DIRS})

SET(SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/hello.c
    ${CMAKE_CURRENT_LIST_DIR}/src/sum.c
    ${CMAKE_CURRENT_LIST_DIR}/src/liblwm2m.h
    ${CMAKE_CURRENT_LIST_DIR}/src/lwm2mclient.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwm2mclient.h
    ${CMAKE_CURRENT_LIST_DIR}/src/gocallbacks.h
    ${CMAKE_CURRENT_LIST_DIR}/src/object_security.c
    ${CMAKE_CURRENT_LIST_DIR}/src/object_server.c
    ${CMAKE_CURRENT_LIST_DIR}/src/object_device.c
    ${CMAKE_CURRENT_LIST_DIR}/src/object_snap_control.c
    ${CMAKE_CURRENT_LIST_DIR}/src/object_snap.c
    ${CMAKE_CURRENT_LIST_DIR}/src/system_api.c
   )

add_library(${PROJECT_NAME} STATIC
      ${SOURCES}
      ${WAKAAMA_SOURCES}
      ${SHARED_SOURCES}
  )
