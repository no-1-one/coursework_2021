project(assembly)
include_directories(${CMAKE_CURRENT_BINARY_DIR})


foreach(_target assembly_server assembly_clientside_test)
    add_executable(${_target} "${_target}.cpp" ${assembly_proto_srcs} ${assembly_grpc_srcs})
    add_dependencies(${_target} assembly_protoc)
    target_link_libraries(${_target} ${_REFLECTION} ${_GRPC_GRPCPP}  ${_PROTOBUF_LIBPROTOBUF} "stdc++")
endforeach()
