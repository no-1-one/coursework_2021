cmake_minimum_required(VERSION 3.14)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_COMPILER /usr/local/bin/g++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lstdc++ -O2")
project(services)

include(FetchContent)
find_package(Threads REQUIRED)
FetchContent_Declare(
  gRPC
  GIT_REPOSITORY https://github.com/grpc/grpc
  GIT_TAG        v1.30.0
  )
set(FETCHCONTENT_QUIET OFF)
FetchContent_MakeAvailable(gRPC)

set(_PROTOBUF_LIBPROTOBUF libprotobuf)
set(_REFLECTION grpc++_reflection)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protoc>)
set(_GRPC_GRPCPP grpc++)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)


set(proto_path "${CMAKE_CURRENT_SOURCE_DIR}/proto")
set(sgp_proto "${CMAKE_CURRENT_SOURCE_DIR}/proto/sgp_service.proto")
set(igrf_proto "${CMAKE_CURRENT_SOURCE_DIR}/proto/igrf_service.proto")
set(assembly_proto "${CMAKE_CURRENT_SOURCE_DIR}/proto/assembly.proto")


set(sgp_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/proto/sgp_service.pb.cc")
set(sgp_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/proto/sgp_service.pb.h")
set(sgp_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/proto/sgp_service.grpc.pb.cc")
set(sgp_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/proto/sgp_service.grpc.pb.h")
set(igrf_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/proto/igrf_service.pb.cc")
set(igrf_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/proto/igrf_service.pb.h")
set(igrf_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/proto/igrf_service.grpc.pb.cc")
set(igrf_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/proto/igrf_service.grpc.pb.h")

set(assembly_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/proto/assembly.pb.cc")
set(assembly_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/proto/assembly.pb.h")
set(assembly_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/proto/assembly.grpc.pb.cc")
set(assembly_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/proto/assembly.grpc.pb.h")



include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/proto)

add_subdirectory(proto)

add_subdirectory(igrf)
add_subdirectory(assembly)
