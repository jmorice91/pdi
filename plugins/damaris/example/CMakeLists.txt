# copy yaml file, to binary folder, for example_damaris_plugin.c
configure_file(example_damaris_plugin.yml example_damaris_plugin.yml COPYONLY)
add_executable (example_damaris_plugin example_damaris_plugin.c)
target_link_libraries (example_damaris_plugin PDI::PDI_C MPI::MPI_CXX)

# copy yaml file, to binary folder, for example_damaris_plugin_with_is_client.c
configure_file(example_damaris_plugin_with_is_client.yml example_damaris_plugin_with_is_client.yml COPYONLY)
add_executable (example_damaris_plugin_with_is_client example_damaris_plugin_with_is_client.c)
target_link_libraries (example_damaris_plugin_with_is_client PDI::PDI_C MPI::MPI_CXX)

# copy xml file for damaris, to binary folder, for example_damaris_only.c
configure_file(example_damaris_api.xml example_damaris_api.xml COPYONLY)
add_executable(example_damaris_only example_damaris_only.c)
target_link_libraries(example_damaris_only ${Damaris_DEPS})