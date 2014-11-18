#include "../include/inncabs.h"

#include "qap.h"

int main(int argc, char** argv) {	
	const char* problem_file = argc>1 ? argv[1] : "input/qap/chr10a.dat";

	// load problem
	problem* p = qap_load(problem_file);

	std::stringstream ss;
	ss << "Quadratic Assignment Solver (" << problem_file << ")";

	inncabs::run_all(
		[&](const std::launch l) {
			return solve(l, p);
		},
		[&](int result) {
			return (result == p->optimum);
		},
		ss.str()
		);

	// free problem
	qap_del(p);
}
