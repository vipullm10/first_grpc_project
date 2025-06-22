cmake_minimum_required(VERSION 3.16)
project(first_grpc_project VERSION 0.1.0 LANGUAGES C CXX)

include(common.cmake)

# Proto file path
set(PROTO_FILE "${CMAKE_SOURCE_DIR}/protos/first_grpc_project.proto")
get_filename_component(PROTO_PATH "${PROTO_FILE}" PATH)

# Where we want generated files to live
set(GEN_SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(GEN_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")

# Output file paths (headers after moving)
set(GEN_PB_CC     "${GEN_SRC_DIR}/first_grpc_project.pb.cc")
set(GEN_GRPC_CC   "${GEN_SRC_DIR}/first_grpc_project.grpc.pb.cc")
set(GEN_PB_H      "${GEN_INCLUDE_DIR}/first_grpc_project.pb.h")
set(GEN_GRPC_H    "${GEN_INCLUDE_DIR}/first_grpc_project.grpc.pb.h")

# Custom command to generate and move files
add_custom_command(
  OUTPUT ${GEN_PB_CC} ${GEN_GRPC_CC} ${GEN_PB_H} ${GEN_GRPC_H}
  COMMAND ${_PROTOBUF_PROTOC}
    --proto_path=${PROTO_PATH}
    --cpp_out=${GEN_SRC_DIR}
    --grpc_out=${GEN_SRC_DIR}
    --plugin=protoc-gen-grpc=${_GRPC_CPP_PLUGIN_EXECUTABLE}
    ${PROTO_FILE}

  # Move headers from src/ to include/
  COMMAND ${CMAKE_COMMAND} -E copy ${GEN_SRC_DIR}/first_grpc_project.pb.h ${GEN_PB_H}
  COMMAND ${CMAKE_COMMAND} -E copy ${GEN_SRC_DIR}/first_grpc_project.grpc.pb.h ${GEN_GRPC_H}
  COMMAND ${CMAKE_COMMAND} -E rm -f ${GEN_SRC_DIR}/first_grpc_project.pb.h
  COMMAND ${CMAKE_COMMAND} -E rm -f ${GEN_SRC_DIR}/first_grpc_project.grpc.pb.h

  DEPENDS ${PROTO_FILE}
  COMMENT "Generating gRPC sources from ${PROTO_FILE}"
)

# Add a dummy target to trigger generation
add_custom_target(generate_grpc_files DEPENDS ${GEN_PB_CC} ${GEN_GRPC_CC} ${GEN_PB_H} ${GEN_GRPC_H})

# Build the gRPC proto library
add_library(fgp_grpc_proto
  ${GEN_PB_CC}
  ${GEN_GRPC_CC}
  ${GEN_PB_H}
  ${GEN_GRPC_H}
)

add_dependencies(fgp_grpc_proto generate_grpc_files)

target_include_directories(fgp_grpc_proto PRIVATE ${GEN_INCLUDE_DIR})
target_link_libraries(fgp_grpc_proto
  absl::check
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
)

# Main app
add_executable(server "${CMAKE_SOURCE_DIR}/src/server.cpp")
add_dependencies(server generate_grpc_files)

target_include_directories(server PRIVATE ${GEN_INCLUDE_DIR})
target_link_libraries(server
  fgp_grpc_proto
)

add_executable(server_async "${CMAKE_SOURCE_DIR}/src/server_async.cpp" "${CMAKE_SOURCE_DIR}/src/socket_client.cpp")
add_dependencies(server_async generate_grpc_files)

target_include_directories(server_async PRIVATE ${GEN_INCLUDE_DIR})
target_link_libraries(server_async
  fgp_grpc_proto
)


add_executable(client "${CMAKE_SOURCE_DIR}/src/client.cpp")
add_dependencies(client generate_grpc_files)

target_include_directories(client PRIVATE ${GEN_INCLUDE_DIR})
target_link_libraries(client
  fgp_grpc_proto
)
