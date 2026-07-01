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

#define IMX 5

const char* CONFIG_FOR_READING_RESULT
	= "logging: trace                                               	\n"
	  "metadata:                                                     	\n"
	  "  nn: int                                                    	\n"
	  "data:                                                       		\n"
	  "  damaris_values: {size: ['$nn'], type: array, subtype: int}     \n"
	  "plugins:                                                       	\n"
	  "  decl_hdf5:                                                   	\n"
	  "    - file: './HDF5_files/distinguish_data_and_dataset_It0.h5'	\n"
	  "      read:                                                    	\n"
	  "        damaris_values:                                          \n"
	  "          dataset: written_values_ds                             \n"
	  "    - file: './HDF5_files/distinguish_data_and_dataset_It0.h5'	\n"
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

	int size = IMX;
	int written_values[size];

	for (int ii = 0; ii < size; ++ii) {
		written_values[ii] = 100 + ii;
	}
	int is_client = 0;
	PDI_expose("is_client", &is_client, PDI_INOUT); // The order doesn't care
	PDI_expose("mpi_comm", &main_comm, PDI_INOUT); // <-- allow plugin to set, returns Damaris client comm

	printf("value of is_client %d=", is_client);
	if (is_client) {
		
		PDI_multi_expose("write", "nn", &size, PDI_OUT, "written_values", written_values, PDI_OUT, NULL);
	}

	PDI_finalize();
	PC_tree_destroy(&conf);

	// comparison of the results

	// reinitialize pdi for reading results from the specified dataset
	PDI_init(PC_parse_string(CONFIG_FOR_READING_RESULT));
	//Let the Damaris server do the work. This way, we can be sure that the file is created correctly and can therefore be read.
	if (!is_client) {
		int damaris_values[size];

		for (int ii = 0; ii < size; ++ii) {
			written_values[ii] = 100 + ii;
			damaris_values[ii] = 6;
		}

        int damaris_size=0;
        PDI_multi_expose("read_size", "nn", &damaris_size, PDI_INOUT, NULL);
        // check that "size == damaris_size"
        if (size != damaris_size) {
			printf("Error: size (= %d) != damaris_size(= %d) \n", size, damaris_size);
			exit(EXIT_FAILURE);
		}
		PDI_multi_expose("read", "damaris_values", damaris_values, PDI_INOUT, NULL);

		for (int ii = 0; ii < size; ++ii) {
			if (written_values[ii] != damaris_values[ii]) {
				printf("written_values[%d] (= %d) != damaris_values[%d] (= %d) \n", ii, written_values[ii], ii, damaris_values[ii]);
				exit(EXIT_FAILURE);
			}
		}
	}
	PDI_finalize();
	MPI_Finalize();

	return EXIT_SUCCESS;
}
