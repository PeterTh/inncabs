#if defined(INNCABS_USE_HPX)
#include <hpx/hpx_main.hpp>
#include <hpx/hpx.hpp>
#endif

#include "../include/inncabs.h"

/*
 * Based on the Barcelona OpenMP Tasks Suite "uts" benchmark
 * Copyright (C) 2009 Barcelona Supercomputing Center - Centro Nacional de Supercomputacion
 * Copyright (C) 2009 Universitat Politecnica de Catalunya
 * Copyright (c) 2007 The Unbalanced Tree Search (UTS) Project Team:
 */

#include <cmath>
#include <vector>

#include "uts.h"

int main(int argc, char** argv) {
	Node root;
	const char *fn = argc > 1 ? argv[1] : "input/uts/test.input";
	uts_read_file(fn);

	std::stringstream ss;
	ss << "Unbalanced Tree Search (" << fn << ")";

	inncabs::run_all(
		[&](const inncabs::launch l) {
			number_of_tasks = parallel_uts(l, &root);
			return number_of_tasks;
		},
		[&](unsigned long long nt) {
			return uts_check_result(nt);
		},
		ss.str(),
		[&] { uts_initRoot(&root); }
		);
    return 0;
}
