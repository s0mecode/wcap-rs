cmake_minimum_required(VERSION 3.15)
project(libwcap_rust CXX)

find_program(CARGO cargo)
if(NOT CARGO)
    message(FATAL_ERROR "Cargo not found. Please install Rust.")
endif()

find_program(CMAKE_CMAKE cmake)

# Build the Rust library
add_custom_target(rust_lib ALL
    COMMAND ${CARGO} build --release
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building Rust bindings..."
)

# Create a custom target that depends on the Rust build
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/target/release/liblibwcap_rs.so
    DEPENDS rust_lib
    COMMAND echo "Rust library built"
)