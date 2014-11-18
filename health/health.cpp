#include "../include/inncabs.h"

/*
* Based on the Barcelona OpenMP Tasks Suite "helath" benchmark
* Copyright (C) 2009 Barcelona Supercomputing Center - Centro Nacional de Supercomputacion
* Copyright (C) 2009 Universitat Politecnica de Catalunya
*/

#include "health.h"

int main(int argc, char** argv) {
	const char* fn = "input/health/test.input";
	if(argc > 1) fn = argv[1];

	std::stringstream ss;
	ss << "Health with input file \"" << fn << "\"";

	struct Village *top;
	read_input_data(fn);

	inncabs::run_all(
		[&](const std::launch l) {
			sim_village_main_par(l, top);
			return 1;
		},
		[&](int result) {
			return check_village(top); 
		},
		ss.str(),
		[&] { allocate_village(&top, NULL, NULL, sim_level, 0); }
		);
}
