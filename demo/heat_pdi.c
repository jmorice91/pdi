/*******************************************************************************
 * Copyright (C) 2015-2026 Commissariat a l'energie atomique et aux energies alternatives (CEA)
 * Copyright (C) 2026 Centre National de la Recherche Scientifique (CNRS)
 * 
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

/*
 * heat_pdi: a standalone 2D heat-equation application that uses PDI as an
 * external library. It is NOT part of the PDI source tree: it is built against
 * an installed PDI + paraconf (see CMakeLists.txt) and everything PDI-related
 * (plugins, data descriptors, checkpointing) is driven by the YAML file given
 * on the command line, under the ".pdi" key.
 *
 * Application-level keys read from the same YAML file:
 *   datasize: [height, width]       global grid size
 *   parallelism: {height, width}    MPI process grid
 *   duration: <seconds>             wall-clock duration of the run
 *   veloc_checkpoint_status: <int>  optional, default 1
 *   temp_threshold: <double>        optional, default 0
 */

#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <paraconf.h>
#include <stdio.h>
#include <stdlib.h>

#include <pdi.h>

/* Called through PDI (e.g. from a user-code plugin): reads shared data and
 * sets "stop" once the coldest interior cell reaches the threshold. */
void check_temperature(void)
{
	double* field;
	int* dsize;
	double* threshold;
	int* stop;
	PDI_access("field", (void**)&field, PDI_IN);
	PDI_access("dsize", (void**)&dsize, PDI_IN);
	PDI_access("threshold", (void**)&threshold, PDI_IN);
	PDI_access("stop", (void**)&stop, PDI_OUT);

	/* smallest temperature over the interior cells (ghosts excluded) */
	double local_min = INFINITY;
	for (int yy = 1; yy < dsize[0] - 1; ++yy) {
		for (int xx = 1; xx < dsize[1] - 1; ++xx) {
			double val = field[yy * dsize[1] + xx];
			if (val < local_min) local_min = val;
		}
	}
	double global_min;
	MPI_Allreduce(&local_min, &global_min, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);

	*stop = (global_min >= *threshold);

	PDI_release("field");
	PDI_release("dsize");
	PDI_release("threshold");
	PDI_release("stop");
}

static void init(int dsize[2], int pcoord[2], double dat[dsize[0]][dsize[1]])
{
	for (int yy = 0; yy < dsize[0]; ++yy) {
		for (int xx = 0; xx < dsize[1]; ++xx) {
			dat[yy][xx] = 0;
		}
	}
	if (pcoord[1] == 0) {
		for (int yy = 0; yy < dsize[0]; ++yy) {
			dat[yy][0] = 1000000;
		}
	}
}

static void iter(int dsize[2], double cur[dsize[0]][dsize[1]], double next[dsize[0]][dsize[1]])
{
	int xx, yy;
	for (xx = 0; xx < dsize[1]; ++xx) {
		next[0][xx] = cur[0][xx];
	}
	for (yy = 1; yy < dsize[0] - 1; ++yy) {
		next[yy][0] = cur[yy][0];
		for (xx = 1; xx < dsize[1] - 1; ++xx) {
			next[yy][xx]
				= (cur[yy][xx] * .5) + (cur[yy][xx - 1] * .125) + (cur[yy][xx + 1] * .125) + (cur[yy - 1][xx] * .125) + (cur[yy + 1][xx] * .125);
		}
		next[yy][dsize[1] - 1] = cur[yy][dsize[1] - 1];
	}
	for (xx = 0; xx < dsize[1]; ++xx) {
		next[dsize[0] - 1][xx] = cur[dsize[0] - 1][xx];
	}
}

static void exchange(MPI_Comm cart_com, int dsize[2], double cur[dsize[0]][dsize[1]])
{
	MPI_Status status;
	int rank_source, rank_dest;
	static MPI_Datatype column, row;
	static int initialized = 0;

	if (!initialized) {
		MPI_Type_vector(dsize[0] - 2, 1, dsize[1], MPI_DOUBLE, &column);
		MPI_Type_commit(&column);
		MPI_Type_contiguous(dsize[1] - 2, MPI_DOUBLE, &row);
		MPI_Type_commit(&row);
		initialized = 1;
	}

	/* send down */
	MPI_Cart_shift(cart_com, 0, 1, &rank_source, &rank_dest);
	MPI_Sendrecv(&cur[dsize[0] - 2][1], 1, row, rank_dest, 100, &cur[0][1], 1, row, rank_source, 100, cart_com, &status);

	/* send up */
	MPI_Cart_shift(cart_com, 0, -1, &rank_source, &rank_dest);
	MPI_Sendrecv(&cur[1][1], 1, row, rank_dest, 100, &cur[dsize[0] - 1][1], 1, row, rank_source, 100, cart_com, &status);

	/* send to the right */
	MPI_Cart_shift(cart_com, 1, 1, &rank_source, &rank_dest);
	MPI_Sendrecv(&cur[1][dsize[1] - 2], 1, column, rank_dest, 100, &cur[1][0], 1, column, rank_source, 100, cart_com, &status);

	/* send to the left */
	MPI_Cart_shift(cart_com, 1, -1, &rank_source, &rank_dest);
	MPI_Sendrecv(&cur[1][1], 1, column, rank_dest, 100, &cur[1][dsize[1] - 1], 1, column, rank_source, 100, cart_com, &status);
}

/* global minimum over interior cells, reduced on main_comm */
static double global_min_temperature(MPI_Comm comm, int dsize[2], double cur[dsize[0]][dsize[1]])
{
	double local_min = INFINITY;
	for (int yy = 1; yy < dsize[0] - 1; ++yy) {
		for (int xx = 1; xx < dsize[1] - 1; ++xx) {
			if (cur[yy][xx] < local_min) local_min = cur[yy][xx];
		}
	}
	double global_min;
	MPI_Allreduce(&local_min, &global_min, 1, MPI_DOUBLE, MPI_MIN, comm);
	return global_min;
}

int main(int argc, char* argv[])
{
	MPI_Init(&argc, &argv);

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <config_file>\n", argv[0]);
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	/* the YAML file holds both the application parameters and, under ".pdi",
	 * the PDI specification tree */
	PC_tree_t conf = PC_parse_path(argv[1]);
	PDI_init(PC_get(conf, ".pdi"));

	MPI_Comm main_comm = MPI_COMM_WORLD;
	long veloc_checkpoint_status = 1;
	double temp_threshold = 0;

	/* optional keys: keep the defaults when they are absent */
	PC_errhandler_t old_handler = PC_errhandler(PC_NULL_HANDLER);
	PC_tree_t status_tree = PC_get(conf, ".veloc_checkpoint_status");
	if (!PC_status(status_tree)) PC_int(status_tree, &veloc_checkpoint_status);
	PC_tree_t threshold_tree = PC_get(conf, ".temp_threshold");
	if (!PC_status(threshold_tree)) PC_double(threshold_tree, &temp_threshold);
	PC_errhandler(old_handler);

	// TODO: if status is recovery, check that MPI comm world is 4 

	int is_client = 1;
	PDI_expose("is_client", &is_client, PDI_INOUT); // The order doesn't care
	PDI_expose("mpi_comm", &main_comm, PDI_INOUT); // <-- allow plugin to set, returns Damaris client comm

	if (is_client) {

	int psize_1d, pcoord_1d;
	MPI_Comm_size(main_comm, &psize_1d);
	MPI_Comm_rank(main_comm, &pcoord_1d);

	PDI_expose("mpi_rank", &pcoord_1d, PDI_OUT);
	PDI_expose("mpi_size", &psize_1d, PDI_OUT);
	PDI_event("init");

	/* application parameters */
	long longval;
	int dsize[2], psize[2];
	double duration;

	PC_int(PC_get(conf, ".datasize[0]"), &longval);
	dsize[0] = longval;
	PC_int(PC_get(conf, ".datasize[1]"), &longval);
	dsize[1] = longval;
	PC_int(PC_get(conf, ".parallelism.height"), &longval);
	psize[0] = longval;
	PC_int(PC_get(conf, ".parallelism.width"), &longval);
	psize[1] = longval;
	PC_double(PC_get(conf, ".duration"), &duration);


	/* local sizes, ghost cells included */
	assert(dsize[0] % psize[0] == 0);
	dsize[0] = dsize[0] / psize[0] + 2;
	assert(dsize[1] % psize[1] == 0);
	dsize[1] = dsize[1] / psize[1] + 2;
	assert(psize[1] * psize[0] == psize_1d);

	int cart_period[2] = {0, 0};
	MPI_Comm cart_com;
	MPI_Cart_create(main_comm, 2, psize, cart_period, 1, &cart_com);
	int pcoord[2];
	MPI_Cart_coords(cart_com, pcoord_1d, 2, pcoord);

	int ii = 0;
	PDI_expose("iter", &ii, PDI_OUT);
	PDI_expose("dsize", dsize, PDI_OUT);
	PDI_expose("psize", psize, PDI_OUT);
	PDI_expose("pcoord", pcoord, PDI_OUT);

	double(*cur)[dsize[1]] = malloc(sizeof(double) * dsize[1] * dsize[0]);
	double(*next)[dsize[1]] = malloc(sizeof(double) * dsize[1] * dsize[0]);

	init(dsize, pcoord, cur);

	double elapsed_offset = 0.0;
	int first_iter = 1;

	PDI_expose("veloc_checkpoint_status", &veloc_checkpoint_status, PDI_OUT);

	double start = MPI_Wtime();
	int next_reduce = 0;
	int stop = 0;

	PDI_event("main_loop");
	for (ii = 0;; ++ii) {
		elapsed_offset = MPI_Wtime() - start;

		/* on restart, the VeloC plugin overwrites these with checkpointed values */
		PDI_multi_expose(
			"newiter",
			"iter", &ii, PDI_INOUT,
			"main_field", cur, PDI_INOUT,
			"elapsed_offset", &elapsed_offset, PDI_INOUT,
			"stop", &stop, PDI_INOUT,
			"temp_threshold", &temp_threshold, PDI_INOUT,
			NULL
		);

		// printf(" iter = %d\n", ii);

		if (first_iter) {
			start = MPI_Wtime() - elapsed_offset;
			first_iter = 0;
		}

		iter(dsize, cur, next);
		exchange(cart_com, dsize, next);
		double(*tmp)[dsize[1]] = cur;
		cur = next;
		next = tmp;

		if (stop) {
			double local_time, global_time;
			local_time = MPI_Wtime() - start;
			MPI_Allreduce(&local_time, &global_time, 1, MPI_DOUBLE, MPI_MAX, main_comm);
			double global_min = global_min_temperature(main_comm, dsize, cur);

			if (0 == pcoord_1d) {
				printf("iter=%7d; time=%7.3f; temperature=%7.3f, TEMPERATURE THRESHOLD REACHED!!!\n", ii, global_time, global_min);
			}
			break;
		}

		if (ii >= next_reduce) {
			double local_time, global_time;
			local_time = MPI_Wtime() - start;
			MPI_Allreduce(&local_time, &global_time, 1, MPI_DOUBLE, MPI_MAX, main_comm);
			if (global_time >= duration) {
				double global_min = global_min_temperature(main_comm, dsize, cur);
				if (0 == pcoord_1d) {
					printf("iter=%7d; time=%7.3f; temperature=%7.3f; END OF SIMULATION!\n", ii, global_time, global_min);
				}
				break;
			}
			int rem_iter = .9 * (duration - global_time) * (ii + 1) / (global_time + 0.1);
			if (rem_iter < 1) rem_iter = 1;
			next_reduce = ii + rem_iter;
			if (0 == pcoord_1d) printf("iter=%7d; time=%7.3f; next_reduce=%7d\n", ii, global_time, next_reduce);
		}
	}
		PDI_event("finalization");
		PDI_expose("iter", &ii, PDI_OUT);
		PDI_expose("main_field", cur, PDI_OUT);

		free(cur);
		free(next);
	}

	PDI_finalize();
	
	PC_tree_destroy(&conf);

	MPI_Finalize();

	return 0;
}