#include "../include/inncabs.h"

/*
 * Based on the Barcelona OpenMP Tasks Suite "floorplan" benchmark
 * Copyright (C) 2009 Barcelona Supercomputing Center - Centro Nacional de Supercomputacion
 * Copyright (C) 2009 Universitat Politecnica de Catalunya
 * Original code from the Cilk project
 * Copyright (c) 1996 Massachusetts Institute of Technology
 */

#include "strassen.h"

int main(int argc, char** argv) {
	arg_size = 1024;
	if(argc > 1) arg_size = atoi(argv[1]);
	arg_cutoff_value = 64;
	if(argc > 2) arg_cutoff_value = atoi(argv[2]);

	if((arg_size & (arg_size - 1)) != 0 || (arg_size % 16) != 0) inncabs::error("Error: matrix size must be a power of 2 and a multiple of 16\n");
	REAL *A = alloc_matrix(arg_size);
	REAL *B = alloc_matrix(arg_size);
	REAL *C = alloc_matrix(arg_size);
	REAL *D = alloc_matrix(arg_size);

	std::stringstream ss;
	ss << "Strassen Algorithm (" << arg_size << " x " << arg_size
		<< " matrix with cutoff " << arg_cutoff_value << ") ";

	init_matrix(arg_size, A, arg_size);
	init_matrix(arg_size, B, arg_size);
	OptimizedStrassenMultiply_seq(D, A, B, arg_size, arg_size, arg_size, arg_size, 1);

	inncabs::run_all(
		[&](const std::launch l) {
			OptimizedStrassenMultiply_par(l, C, A, B, arg_size, arg_size, arg_size, arg_size, 1);
			return 1;
		},
		[&](int result) {
			return compare_matrix(arg_size, C, arg_size, D, arg_size);
		},
		ss.str()
		);
}
