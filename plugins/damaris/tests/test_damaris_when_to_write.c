/*******************************************************************************
 * Copyright (C) 2026 Commissariat a l'energie atomique et aux energies alternatives (CEA)
 * Copyright (C) 2026 Institut National de Recherche en Sciences et Technologies du Numérique (INRIA)
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
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <pdi.h>

#include <unistd.h>
#include <sys/stat.h>

#define TOTAL_SIZE 10
#define ITER_LIMIT 5

const char* CONFIG_FOR_READING_RESULT
	= "logging: trace                                               	\n"
	  "metadata:                                                     	\n"
	  "  nn: int                                                    	\n"
	  "  iter_limit: int                                              	\n"
	  "data:                                                       		\n"
	  "  damaris_values: {size: ['$nn'], type: array, subtype: int}     \n"
	  "plugins:                                                       	\n"
	  "  decl_hdf5:                                                   	\n"
	  "    - file: './HDF5_files/when_to_write_It$($iter_limit-1).h5'	\n"
	  "      read:                                                    	\n"
	  "        damaris_values:                                          \n"
	  "          dataset: written_values_ds                             \n"
	  "    - file: './HDF5_files/when_to_write_It$($iter_limit-1).h5'	\n"
	  "      on_event: 'read_size'										\n"
	  "      read:														\n"
	  "        nn:														\n"
	  "          size_of: written_values_ds								\n";

int main(int argc, char* argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <config_file>\n", argv[0]);
		exit(1);
	}
	MPI_Init(&argc, &argv);

	MPI_Comm main_comm = MPI_COMM_WORLD;
	int world_size;
	MPI_Comm_size(main_comm, &world_size);
	if (world_size != 2) {
		fprintf(stderr, "Please use at least 2 mpi processes\n");
		exit(1);
	}

	// get specification tree
	PC_tree_t conf = PC_parse_path(argv[1]);

	// initialize pdi
	PDI_init(PC_get(conf, ".pdi"));

	// All processes must initialize Damaris with the XML configuration
	//  - client process = heat simulation process
	//  - server process = damaris process for writting hdf5 file.

	int size = TOTAL_SIZE;
	int iter_limit = ITER_LIMIT;
	int written_values[size];

	int is_client = 0;
	PDI_expose("is_client", &is_client, PDI_INOUT); // The order doesn't care
	PDI_expose("mpi_comm", &main_comm, PDI_INOUT); // <-- allow plugin to set, returns Damaris client comm

	PDI_multi_expose("write", "nn", &size, PDI_OUT, "iter_limit", &iter_limit, PDI_OUT, NULL);

	printf("value of is_client %d=", is_client);
	if (is_client) {		
		int ii;
		for (ii = 0; ii < size; ++ii) {
			written_values[ii] = 100 + ii;
			PDI_multi_expose("write", "iter", &ii, PDI_OUT, "written_values", written_values, PDI_OUT, NULL);
		}
		PDI_expose("iter", &ii, PDI_OUT);
	}

	PDI_finalize();
	MPI_Finalize();
	PC_tree_destroy(&conf);

	// comparison of the results

	// Check that the "when" clause worked as expected:
	// only iterations 0..iter_limit-1 should have produced a file
	if (!is_client) {
		char filename[256];
		for (int it = 0; it < size; ++it) {
			snprintf(filename, sizeof(filename), "./HDF5_files/when_to_write_It%d.h5", it);
			struct stat buffer;
			int file_exists = (stat(filename, &buffer) == 0);
			int should_exist = (it < iter_limit);

			if (file_exists != should_exist) {
				printf("Error: file '%s' exists=%d but expected exists=%d (it=%d, iter_limit=%d)\n",
					filename, file_exists, should_exist, it, iter_limit);
				exit(EXIT_FAILURE);
			}
		}
	}

	return EXIT_SUCCESS;
}
