#include "../include/inncabs.h"

/*
 * Based on the Barcelona OpenMP Tasks Suite "sort" benchmark
 * Copyright (C) 2009 Barcelona Supercomputing Center - Centro Nacional de Supercomputacion
 * Copyright (C) 2009 Universitat Politecnica de Catalunya
 * Original code from the Cilk project
 * Copyright (c) 2000 Massachusetts Institute of Technology
 * Copyright (c) 2000 Matteo Frigo
 */

#include "sort.h"

int main(int argc, char** argv) {
	arg_size = 10000;
	if(argc > 1) arg_size = atoi(argv[1]);
	arg_cutoff_1 = 512;
	if(argc > 2) arg_cutoff_1 = atoi(argv[2]);
	arg_cutoff_2 = 512;
	if(argc > 3) arg_cutoff_2 = atoi(argv[3]);
	arg_cutoff_3 = 128;
	if(argc > 4) arg_cutoff_3 = atoi(argv[4]);

	std::stringstream ss;
	ss << "Sort with N = " << arg_size << ", cutoffs = " << arg_cutoff_1 << " / " << arg_cutoff_2 << " / " << arg_cutoff_3;
	
	inncabs::run_all(
		[&](const std::launch l) {
			sort_par(l);
			return 1;
		},
		[&](int result) {
			return sort_verify(); 
		},
		ss.str(),
		[&] { sort_init(); }
		);
}
