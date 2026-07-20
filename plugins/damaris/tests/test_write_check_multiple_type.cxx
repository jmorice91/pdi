/*******************************************************************************
 * Copyright (C) 2026 Commissariat a l'energie atomique et aux energies alternatives (CEA)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the name of CEA nor the names of its contributors may be used to
 *   endorse or promote products derived from this software without specific
 *   prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include <mpi.h>
#include <stdio.h>
#include <unistd.h>
#include <pdi.h>

#include <algorithm>
#include <filesystem>
#include <random>
#include <ranges>
#include <type_traits>
#include <iostream>

#include "multiple_check_include.h"
#include <pdi.h>

constexpr char CONFIG_YAML[] = R"(
logging: debug
pdi:
  metadata:
    nn: int
    mpi_comm: MPI_Comm
    is_client: int
  data:
    MY8TYPE_values: {size: ["$nn"], type: array, subtype: MY8TYPE}
  plugins:
    trace: info
    mpi:
    decl_hdf5:
      - file: 'data_iter0.h5'
        communicator: $mpi_comm
        write: [MY8TYPE_values]
        on_event: write
    damaris:
      architecture:
        sim_name: damaris_scalar_type
        domains: 1
        dedicated:
          core: 1
          node: 0
      get_is_client: is_client
      client_comm_get: mpi_comm
      datasets:
        - dataset:
            name: MY8TYPE_values
            layout: MY8TYPE_values_layout
            storage: hdf5_storage
      layouts:
        - layout:
            name: MY8TYPE_values_layout
            type: MY8TYPE
            global: ["$nn"]
            dimensions: ["$nn"]
            ghosts: '0:0'
            depends_on: [nn]
      storages:
        - storage:
            name: hdf5_storage
            type: HDF5
            file_mode: Collective
            files_path: ./HDF5_files/
      write: 
        MY8TYPE_values:
          dataset: MY8TYPE_values
          position: ['0']
      log:
        rotation_size: 5
        log_level: info
        flush: true
)";

template <typename T>
void write_file(const char *CONFIG_FILE, const std::string & name_var) {

  if (type_name<T>() != name_var) {
    throw std::runtime_error("The type name is not corresponding to it string name, type_name<T>() != name_var:" + type_name<T>() + " != " + name_var);
  }

  MPI_Comm main_comm = MPI_COMM_WORLD;
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	if (world_size != 2) {
		fprintf(stderr, "Please use at least 2 mpi processes\n");
		exit(1);
	}
	PC_tree_t conf = PC_parse_string(CONFIG_FILE);

	// initialize pdi
	PDI_init(PC_get(conf, ".pdi"));

	// All processes must initialize Damaris with the XML configuration
	//  - client process = heat simulation process
	//  - server process = damaris process for writting hdf5 file.

	int is_client = 0;
	PDI_expose("is_client", &is_client, PDI_INOUT); // The order doesn't care
	PDI_expose("mpi_comm", &main_comm, PDI_INOUT); // <-- allow plugin to set, returns Damaris client comm

	if (is_client) {
    std::mt19937_64 m_random_generator{1010};
    const int IMX = 10;
    int size = IMX;
    auto my_values = make_random<std::array<T,IMX>>(m_random_generator);

    std::string data_to_write=name_var+"_values";
    
		PDI_expose("nn", &size, PDI_INOUT);
		PDI_multi_expose("write", data_to_write.c_str(), my_values.data(), PDI_OUT, NULL);
	}

	PDI_finalize();
	PC_tree_destroy(&conf);
}

void run_test(const std::string & my_type_name,const char *CONFIG_TO_CHANGE) {
	std::string my_new_config = replace_all(std::string(CONFIG_TO_CHANGE),std::string("MY8TYPE"),my_type_name);

	if (my_type_name == "short") {
		write_file<short>(my_new_config.c_str(), my_type_name);
	} else if (my_type_name == "int") {
		write_file<int>(my_new_config.c_str(), my_type_name);
	} else if (my_type_name == "long") {
		write_file<long>(my_new_config.c_str(), my_type_name);
	} else if (my_type_name == "float") {
		write_file<float>(my_new_config.c_str(), my_type_name);
	} else if (my_type_name == "double") {
		write_file<double>(my_new_config.c_str(), my_type_name);
	} else {
		throw std::runtime_error("This type is not supported");
	}
}

int main(int argc, char* argv[])
{
	MPI_Init(&argc, &argv);

	run_test(argv[1],CONFIG_YAML);

	MPI_Finalize();

	return EXIT_SUCCESS;
}
