cmake_minimum_required(VERSION 3.22...4.2)
project(heat_pdi LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)


# PDI and paraconf are found as installed packages
# not built as part of this project

find_package(MPI REQUIRED COMPONENTS C)
find_package(paraconf REQUIRED COMPONENTS C)
find_package(PDI REQUIRED COMPONENTS C)

add_executable(heat_pdi heat_pdi.c)
target_link_libraries(heat_pdi PRIVATE MPI::MPI_C paraconf::paraconf PDI::PDI_C m)
set_target_properties(heat_pdi PROPERTIES ENABLE_EXPORTS TRUE)