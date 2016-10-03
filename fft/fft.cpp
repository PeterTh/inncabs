#if defined(INNCABS_USE_HPX)
#include <hpx/hpx_main.hpp>
#include <hpx/hpx.hpp>
#endif

#include "../include/inncabs.h"

/*
 * Original code from the Cilk project
 * Copyright (c) 2000 Massachusetts Institute of Technology
 * Copyright (c) 2000 Matteo Frigo
 */

#include "fft.h"

int main(int argc, char** argv) {
	int n = 1000;
	if(argc > 1) n = atoi(argv[1]);

	std::stringstream ss;
	ss << "FFT N=" << n;

	COMPLEX *in = NULL, *out_seq = NULL, *out = NULL;
	in = (COMPLEX*)malloc(n * sizeof(COMPLEX));
	out_seq = (COMPLEX*)malloc(n * sizeof(COMPLEX));
	out = (COMPLEX*)malloc(n * sizeof(COMPLEX));

	fft_init(n, in);
	fft_seq(n, in, out_seq);

	inncabs::run_all(
		[&](const inncabs::launch l) {
			fft(l, n, in, out);
			return 1;
		},
		[&](int result) {
			return test_correctness(n, out, out_seq);
		},
		ss.str(),
		[&] { fft_init(n, in); }
		);

	free(in);
	free(out_seq);
	free(out);

    return 0;
}
