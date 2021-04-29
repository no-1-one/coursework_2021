project(igrf)




include_directories(${CMAKE_CURRENT_BINARY_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/sgp4)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/sgp4/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/igrf)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/igrf/include)
set(_LIBSGP4 ${CMAKE_CURRENT_SOURCE_DIR}/sgp4/libsgp4.a)
set(_LIBIGRF ${CMAKE_CURRENT_SOURCE_DIR}/igrf/libigrf.so)

set(_IGRFCOF ${CMAKE_CURRENT_SOURCE_DIR}/IGRF13.COF)

add_custom_target(copy_cof ALL
  COMMAND cp ${_IGRFCOF} ${CMAKE_CURRENT_BINARY_DIR}
)

foreach(_target igrf_server)
    add_executable(${_target} "${_target}.cpp" "noise_application.cpp" ${sgp_proto_srcs} ${sgp_grpc_srcs} ${igrf_proto_srcs} ${igrf_grpc_srcs})
    add_dependencies(${_target} sgp_protoc igrf_protoc)
    target_link_libraries(${_target} ${_REFLECTION} ${_GRPC_GRPCPP}  ${_PROTOBUF_LIBPROTOBUF} ${_LIBSGP4} ${_LIBIGRF} "m")
endforeach() 

