#include "../include/inncabs.h"

/*
 * Based on the Barcelona OpenMP Tasks Suite "floorplan" benchmark
 * Copyright (C) 2009 Barcelona Supercomputing Center - Centro Nacional de Supercomputacion
 * Copyright (C) 2009 Universitat Politecnica de Catalunya
 * Original code from the Application Kernel Matrix by Cray
 */

#include "floorplan.h"

int main(int argc, char** argv) {
	const char* fn = "input/floorplan/input.5";
	if(argc > 1) fn = argv[1];

	std::stringstream ss;
	ss << "Floorplan with input file \"" << fn << "\"";
	
	inncabs::run_all(
		[&](const std::launch l) {
			compute_floorplan(l);
			return 1;
		},
		[&](int result) {
			return floorplan_verify(); 
		},
		ss.str(),
		[&] { floorplan_init(fn); }
		);
}
