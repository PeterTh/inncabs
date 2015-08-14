#if defined(INNCABS_USE_HPX)
#include <hpx/hpx_main.hpp>
#include <hpx/hpx.hpp>
#if defined(INNCABS_USE_HPX_FUTURIZED)
#include <hpx/include/parallel_for_each.hpp>
#include <hpx/include/parallel_executor_parameters.hpp>
#endif
#endif

#include "../include/inncabs.h"

/*
 * Based on the Barcelona OpenMP Tasks Suite "sparselu" benchmark
 * Copyright (C) 2009 Barcelona Supercomputing Center - Centro Nacional de Supercomputacion
 * Copyright (C) 2009 Universitat Politecnica de Catalunya
 */

#include "sparselu.h"

int main(int argc, char** argv) {
	arg_size_1 = 50;
	if(argc > 1) arg_size_1 = atoi(argv[1]);
	arg_size_2 = 100;
	if(argc > 2) arg_size_2 = atoi(argv[2]);

	std::stringstream ss;
	ss << "SparseLU Factorization (" << arg_size_1 << "x" << arg_size_1
	   << " matrix with " << arg_size_2 << "x" << arg_size_2 << " blocks) ";

	sparselu_init(&SEQ, "serial");
	sparselu_seq_call(SEQ);

	inncabs::run_all(
		[&](const inncabs::launch l) {
			sparselu_par_call(l, BENCH);
			return 1;
		},
		[&](int result) {
			return sparselu_check(SEQ, BENCH);
		},
		ss.str(),
		[&] { sparselu_init(&BENCH, "benchmark"); }
		);
    return 0;
}
