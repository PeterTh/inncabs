#if defined(INNCABS_USE_HPX)
#include <hpx/hpx_main.hpp>
#include <hpx/hpx.hpp>
#endif

#include "../include/inncabs.h"

#include "pyramids.h"

int main(int argc, char** argv) {

	// allocate two copies of the processed array
	Grid* A = (Grid*)malloc(sizeof(Grid));
	Grid* B = (Grid*)malloc(sizeof(Grid));

	std::stringstream ss;
	ss << "Cache-oblivious Jacobi Solver (P = " << P << " = " << N << " x " << M << "; CUT = " << CUT << ")";

	inncabs::run_all(
		[&](const inncabs::launch l) {
			jacobi_recursive(l,A,B,M);
			return 1;
		},
		[&](int result) {
			return jacobi_verify(A);
		},
		ss.str(),
		[&] { jacobi_init(A); }
		);
    return 0;
}
