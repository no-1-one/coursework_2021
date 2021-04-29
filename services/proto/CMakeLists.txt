project(protos)

add_custom_target(sgp_protoc ALL
    #OUTPUT "${sgp_proto_srcs}" "${sgp_proto_hdrs}" "${sgp_grpc_srcs}" "${sgp_grpc_hdrs}"
    COMMAND ${_PROTOBUF_PROTOC}
         --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
         --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
         -I "${proto_path}"
         --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
         "${sgp_proto}"
)


add_custom_target(igrf_protoc ALL
    #OUTPUT "${igrf_proto_srcs}" "${igrf_proto_hdrs}" "${igrf_grpc_srcs}" "${igrf_grpc_hdrs}"
    COMMAND ${_PROTOBUF_PROTOC}
         --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
         --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
         -I "${proto_path}"
         --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
         "${igrf_proto}"
)


add_custom_target(assembly_protoc ALL
    #OUTPUT "${igrf_proto_srcs}" "${igrf_proto_hdrs}" "${igrf_grpc_srcs}" "${igrf_grpc_hdrs}"
    COMMAND ${_PROTOBUF_PROTOC}
         --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
         --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
         -I "${proto_path}"
         --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
         "${assembly_proto}"
)
