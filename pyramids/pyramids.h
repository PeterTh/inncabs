#pragma once

#include "allscale/api/core/prec.h"

/*
 Adapted from the Insieme compiler "pyramids" test case.
 Copyright 2012-2014 University of Innsbruck
 */

#include <cmath>
#include <cassert>

 // define the problem size
#ifndef P
#define P 11
#endif

// has to be one of 2^n + 1
#define N ((1<<P)+1)

// defines the number of iterations
#define M ((N-1)/2)


// define the cut-off value
#ifndef CUT
#define CUT 6
#endif

#define CUT_OFF ((1<<CUT)-1)

// the type of value the computation should be based on
#ifndef VALUE_TYPE
#define VALUE_TYPE double
#endif

// enables debugging messages
#ifndef DEBUG
#define DEBUG 0
#endif

typedef VALUE_TYPE real;
typedef real Grid[N][N];

#define min(A,B) (((A)<(B))?(A):(B))

void printGrid(Grid* A) {
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			printf((A[0][i][j] != 0.0) ? "+" : "-");
			//printf("%d",((int)A[0][i][j])%10);
		}
		printf("\n");
	}
	printf("Center: %f\n", A[0][N / 2][N / 2]);
}

// -----------------------------------------------------
// Stencil Kernel
// -----------------------------------------------------

int steps;

void update(Grid* A, Grid* B, int i, int j) {

	// correct coordinates
	i = (i + (N - 1)) % (N - 1);
	j = (j + (N - 1)) % (N - 1);

	//	printf("Location: %d/%d\n", i, j);

		// test for edge coordinates
	if (i == 0 || j == 0 || i == N - 1 || j == N - 1) {
		return;		// nothing to do on edge (so far)
	}

	(*B)[i][j] = (double)1 / 4 * ((*A)[i - 1][j] + (*A)[i][j + 1] + (*A)[i][j - 1] + (*A)[i + 1][j]);

	//	// ---- Debugging ----
	//
	//	int s = A[0][i][j];
	//	assert(0 <= i && i < N);
	//	assert(0 <= j && j < N);
	//
	//	steps++;
	//
	//	if (
	//			(i!=1 && A[0][i-1][j] != s) ||
	//			(j!=1 && A[0][i][j-1] != s) ||
	//			(i!=N-2 && A[0][i+1][j] != s) ||
	//			(j!=N-2 && A[0][i][j+1] != s) ||
	//			A[0][i][j] != s) {
	//
	//		printf("Input data not valid: %d / %d / %d / %d / %d - should: %d\n",
	//				(int)A[0][i-1][j], (int)A[0][i][j-1], (int)A[0][i+1][j], (int)A[0][i][j+1], (int)A[0][i][j], s);
	//
	//		printf("Step %d: Problem %d/%d/%d ...\n", steps, i, j, s+1);
	//		printf("\nA:\n"); printGrid(A);
	//		printf("\nB:\n"); printGrid(B);
	//		assert(0);
	//	}
	//
	//	if (B[0][i][j] >= s + 1) {
	//		printf("Step %d: Re-computing point %d/%d/%d\n", steps, i, j, s+1);
	//		printf("\nA:\n"); printGrid(A);
	//		printf("\nB:\n"); printGrid(B);
	//		assert(0);
	//	}
	//
	//	// count updates
	//	//printf("%d - Computing (%d,%d,%d)\n", steps++, i, j, s+1);
	//	B[0][i][j] = s + 1;

}

// -----------------------------------------------------
// Iterative Computation
// -----------------------------------------------------
void jacobi_iterative(Grid* A, Grid* B, int num_iter) {
	// #pragma omp parallel
	for (int step = 0; step < num_iter; step++) {
		// #pragma omp for
		for (int i = 1; i < N - 1; i++) {
			for (int j = 1; j < N - 1; j++) {
				update(A, B, i, j);
			}
		}
		// switch sides
		Grid* C = A;
		A = B;
		B = C;
	}
}


// -----------------------------------------------------
// Recursive Computation
// -----------------------------------------------------

struct params {
	Grid* A;
	Grid* B;
	int x;
	int y;
	int s;
};

// computes a pyramid types
template<typename Comp_P, typename Comp_R, typename Comp_WX, typename Comp_WY>
void compute_pyramid(const params& p, const Comp_P& compute_pyramid_step, const Comp_R& compute_reverse_step, const Comp_WX& compute_wedge_x_step, const Comp_WY& compute_wedge_y_step);
template<typename Comp_P, typename Comp_R, typename Comp_WX, typename Comp_WY>
void compute_reverse(const params& p, const Comp_P& compute_pyramid_step, const Comp_R& compute_reverse_step, const Comp_WX& compute_wedge_x_step, const Comp_WY& compute_wedge_y_step);

// computes a wedge (piece between pyramids - x base line points in x direction)
template<typename Comp_P, typename Comp_R, typename Comp_WX, typename Comp_WY>
void compute_wedge_x(const params& p, const Comp_P& compute_pyramid_step, const Comp_R& compute_reverse_step, const Comp_WX& compute_wedge_x_step, const Comp_WY& compute_wedge_y_step);
template<typename Comp_P, typename Comp_R, typename Comp_WX, typename Comp_WY>
void compute_wedge_y(const params& p, const Comp_P& compute_pyramid_step, const Comp_R& compute_reverse_step, const Comp_WX& compute_wedge_x_step, const Comp_WY& compute_wedge_y_step);

/**
 * Computes the pyramid with center point (x,y) of size s (edge size, must be odd)
 */
template<typename Comp_P, typename Comp_R, typename Comp_WX, typename Comp_WY>
void compute_pyramid(const params& p, const Comp_P& compute_pyramid_step, const Comp_R& compute_reverse_step, const Comp_WX& compute_wedge_x_step, const Comp_WY& compute_wedge_y_step) {
	Grid* A = p.A;
	Grid* B = p.B;
	int x = p.x;
	int y = p.y;
	int s = p.s;

	assert(s % 2 == 1 && "Only odd sizes are supported!");
	//assert(x >= s && y >= s && "Coordinates not matching!");

	if (DEBUG) printf("Decomposing pyramid at (%d,%d) with size %d ...\n", x, y, s);

	// cut into 6 smaller pyramids + 4 wedges

	// compute size of sub-pyramids
	int d = (s - 1) / 2;
	int h = (d + 1) / 2;

	int ux = x - h;
	int lx = x + h;
	int uy = y - h;
	int ly = y + h;

    allscale::api::core::sequential(

    	// compute 4 base-pyramids (parallel)
        allscale::api::core::parallel(
	        compute_pyramid_step({ A, B, ux, uy, d }),
            compute_pyramid_step({ A, B, ux, ly, d }),
            compute_pyramid_step({ A, B, lx, uy, d }),
            compute_pyramid_step({ A, B, lx, ly, d })
        ),

        // compute 4 wedges (parallel)
        allscale::api::core::parallel(
	        compute_wedge_x_step({ A, B, ux, y, d }),
            compute_wedge_x_step({ A, B, lx, y, d }),
            compute_wedge_y_step({ A, B, x, uy, d }),
            compute_wedge_y_step({ A, B, x, ly, d })
        ),

    	// compute reverse pyramid in the center
	    compute_reverse_step({ A, B, x, y, d }),

	    // compute tip
	    compute_pyramid_step({ A, B, x, y, d })

    ).get();

}

void compute_pyramid_base(const params& p) {
	Grid* A = p.A;
	Grid* B = p.B;
	int x = p.x;
	int y = p.y;
	int s = p.s;

	int h = (s + 1) / 2;
	if (DEBUG) printf("Computing pyramid at (%d,%d) with size %d and %d levels ...\n", x, y, s, h);

	// just compute the pyramid
	for (int l = h - 1; l >= 0; l--) {

		// compute one plain of the pyramid
		for (int i = x - l; i <= x + l; i++) {
			for (int j = y - l; j <= y + l; j++) {
				update(A, B, i, j);
			}
		}

		// switch plains
		Grid* C = A;
		A = B;
		B = C;
	}
}

template<typename Comp_P, typename Comp_R, typename Comp_WX, typename Comp_WY>
void compute_reverse(const params& p, const Comp_P& compute_pyramid_step, const Comp_R& compute_reverse_step, const Comp_WX& compute_wedge_x_step, const Comp_WY& compute_wedge_y_step) {
	Grid* A = p.A;
	Grid* B = p.B;
	int x = p.x;
	int y = p.y;
	int s = p.s;

	assert(s % 2 == 1 && "Only odd sizes are supported!");

	if (DEBUG) printf("Decomposing reverse pyramid at (%d,%d) with size %d ...\n", x, y, s);

	// cut into 6 smaller pyramids + 4 wedges

	// compute size of pyramids
	int d = (s - 1) / 2;
	int h = (d + 1) / 2;

	int ux = x - h;
	int lx = x + h;
	int uy = y - h;
	int ly = y + h;

    allscale::api::core::sequential(

	    // compute tip
	    compute_reverse_step({A, B, x, y, d}),

	    // compute reverse pyramid in the center
	    compute_pyramid_step({A, B, x, y, d}),


    	// compute 4 wedges (parallel)
        allscale::api::core::parallel(
	        compute_wedge_y_step({ A, B, ux, y, d}),
            compute_wedge_y_step({ A, B, lx, y, d}),
	        compute_wedge_x_step({ A, B, x, uy, d}),
	        compute_wedge_x_step({ A, B, x, ly, d})
        ),

    	// compute 4 base-pyramids (parallel)
        allscale::api::core::parallel(
	        compute_reverse_step({ A, B, lx, ly, d}),
	        compute_reverse_step({ A, B, lx, uy, d}),
	        compute_reverse_step({ A, B, ux, ly, d}),
	        compute_reverse_step({ A, B, ux, uy, d})
        )

    ).get();
}


void compute_reverse_base(const params& p) {
	Grid* A = p.A;
	Grid* B = p.B;
	int x = p.x;
	int y = p.y;
	int s = p.s;

	// compute height of pyramid
	int h = (s + 1) / 2;
	if (DEBUG) printf("Computing reverse pyramid at (%d,%d) with size %d  ...\n", x, y, s);

	// just compute the pyramid
	for (int l = 0; l < h; l++) {

		// compute one plain of the pyramid
		for (int i = x - l; i <= x + l; i++) {
			for (int j = y - l; j <= y + l; j++) {
				update(A, B, i, j);
			}
		}

		// switch plains
		Grid* C = A;
		A = B;
		B = C;
	}
}


template<typename Comp_P, typename Comp_R, typename Comp_WX, typename Comp_WY>
void compute_wedge_x(const params& p, const Comp_P& compute_pyramid_step, const Comp_R& compute_reverse_step, const Comp_WX& compute_wedge_x_step, const Comp_WY& compute_wedge_y_step) {
	Grid* A = p.A;
	Grid* B = p.B;
	int x = p.x;
	int y = p.y;
	int s = p.s;

	assert(s > 0);

	if (DEBUG) printf("Decomposing wedge %d/%d/%d/X ...\n", x, y, s);

	// decompose into 2 pyramids and 4 wedges

	// compute coordinates offset of sub-wedges
	int d = (s - 1) / 2;
	int h = (d + 1) / 2;

    allscale::api::core::sequential(

    	// compute bottom wedges (parallel)
        allscale::api::core::parallel(
            compute_wedge_x_step({A, B, x - h, y, d}),
            compute_wedge_x_step({A, B, x + h, y, d})
        ),

    	// reverse pyramid
        compute_reverse_step({A, B, x, y, d}),

    	// compute pyramid on top
        compute_pyramid_step({A, B, x, y, d}),

    	// compute remaining two wedges (parallel)
        allscale::api::core::parallel(
            compute_wedge_x_step({A, B, x, y - h, d}),
            compute_wedge_x_step({A, B, x, y + h, d})
        )

    ).get();
}

void compute_wedge_x_base(const params& p) {
	Grid* A = p.A;
	Grid* B = p.B;
	int x = p.x;
	int y = p.y;
	int s = p.s;

	// compute height of wedge
	int h = (s + 1) / 2;

	if (DEBUG) printf("Computing wedge %d/%d/%d/X - height %d ...\n", x, y, s, h);

	// just compute the wedge
	for (int l = 0; l < h; l++) {

		// compute one level of the wedge
		for (int i = x - (h - l) + 1; i <= x + (h - l) - 1; i++) {
			for (int j = y - l; j <= y + l; j++) {
				update(A, B, i, j);
			}
		}

		// switch plains
		Grid* C = A;
		A = B;
		B = C;
	}
}

template<typename Comp_P, typename Comp_R, typename Comp_WX, typename Comp_WY>
void compute_wedge_y(const params& p, const Comp_P& compute_pyramid_step, const Comp_R& compute_reverse_step, const Comp_WX& compute_wedge_x_step, const Comp_WY& compute_wedge_y_step) {
	Grid* A = p.A;
	Grid* B = p.B;
	int x = p.x;
	int y = p.y;
	int s = p.s;

	assert(s > 0);

	if (DEBUG) printf("Decomposing wedge %d/%d/%d/Y ...\n", x, y, s);

	// decompose into 2 pyramids and 4 wedges

	// compute coordinates offset of sub-wedges
	int d = (s - 1) / 2;
	int h = (d + 1) / 2;

    // run sub-tasks
    allscale::api::core::sequential(

    	// compute bottom wedges (parallel)
        allscale::api::core::parallel(
            compute_wedge_y_step({A, B, x, y - h, d}),
            compute_wedge_y_step({A, B, x, y + h, d})
        ),

    	// reverse pyramid
    	compute_reverse_step({A, B, x, y, d}),

    	// compute pyramid on top
    	compute_pyramid_step({A, B, x, y, d}),

        // compute remaining two wedges (parallel)
        allscale::api::core::parallel(
            compute_wedge_y_step({A, B, x - h, y, d}),
            compute_wedge_y_step({A, B, x + h, y, d})
        )

    ).get();
}

void compute_wedge_y_base(const params& p) {
	Grid* A = p.A;
	Grid* B = p.B;
	int x = p.x;
	int y = p.y;
	int s = p.s;

	// compute height of wedge
	int h = (s + 1) / 2;

	if (DEBUG) printf("Computing wedge %d/%d/%d/Y ...\n", x, y, s);

	// just compute the wedge
	for (int l = 0; l < h; l++) {

		// compute one plain of the pyramid
		for (int i = x - l; i <= x + l; i++) {
			//printf("Level %d - bounds x: %d,%d - bounds y: %d,%d\n", h, x-l, x+l, y-(h-l)+1, y+(h-l)-1 );
			for (int j = y - (h - l) + 1; j <= y + (h - l) - 1; j++) {
				update(A, B, i, j);
			}
		}

		// switch plains
		Grid* C = A;
		A = B;
		B = C;
	}
}


void jacobi_recursive(const std::launch l, Grid* A, Grid* B, int num_iter) {

	auto jacobi_group = allscale::api::core::group(
		allscale::api::core::fun( // compute_pyramid_step
			[](const params& p){ return p.s <= CUT_OFF; },
			[](const params& p){ compute_pyramid_base(p); },
			[](const params& p, const auto& compute_pyramid_step, const auto& compute_reverse_step, const auto& compute_wedge_x_step, const auto& compute_wedge_y_step){
				compute_pyramid(p, compute_pyramid_step, compute_reverse_step, compute_wedge_x_step, compute_wedge_y_step);
			}
		),
		allscale::api::core::fun( // compute_reverse_step
			[](const params& p){ return p.s <= CUT_OFF; },
			[](const params& p){ compute_reverse_base(p); },
			[](const params& p, const auto& compute_pyramid_step, const auto& compute_reverse_step, const auto& compute_wedge_x_step, const auto& compute_wedge_y_step){
				compute_reverse(p, compute_pyramid_step, compute_reverse_step, compute_wedge_x_step, compute_wedge_y_step);
			}
		),
		allscale::api::core::fun( // compute_wedge_x_step
			[](const params& p){ return p.s <= CUT_OFF; },
			[](const params& p){ compute_wedge_x_base(p); },
			[](const params& p, const auto& compute_pyramid_step, const auto& compute_reverse_step, const auto& compute_wedge_x_step, const auto& compute_wedge_y_step){
				compute_wedge_x(p, compute_pyramid_step, compute_reverse_step, compute_wedge_x_step, compute_wedge_y_step);
			}
		),
		allscale::api::core::fun( // compute_wedge_y_step
			[](const params& p){ return p.s <= CUT_OFF; },
			[](const params& p){ compute_wedge_y_base(p); },
			[](const params& p, const auto& compute_pyramid_step, const auto& compute_reverse_step, const auto& compute_wedge_x_step, const auto& compute_wedge_y_step){
				compute_wedge_y(p, compute_pyramid_step, compute_reverse_step, compute_wedge_x_step, compute_wedge_y_step);
			}
		)
	);

	// compute full pyramid
	inncabs::message("\nProcessing main pyramid ...\n");
	allscale::api::core::prec<0>(jacobi_group)({ A, B, N / 2, N / 2, N - 2 }).get();
	inncabs::message("\nProcessing x wedge ...\n");
	allscale::api::core::prec<2>(jacobi_group)({ A, B, N / 2,0, N - 2 }).get();
	inncabs::message("\nProcessing y wedge ...\n");
	allscale::api::core::prec<3>(jacobi_group)({ A, B, 0, N / 2, N - 2 }).get();
	inncabs::message("\nProcessing reverse pyramid ...\n");
	allscale::api::core::prec<1>(jacobi_group)({ A, B, 0, 0, N - 2 }).get();
}

void jacobi_init(Grid* A) {
	// fill input data into A
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			(*A)[i][j] = 0;
		}
	}

	// light a candle in the center
	(*A)[N / 2][N / 2] = 1;
}

bool jacobi_verify(Grid* A) {
	double sum = 0.0;
	bool ok = true;
	for (int i = 0; i<N && ok; ++i) {
		for (int j = 0; j<N && ok; ++j) {
			sum += A[0][i][j];
			if (labs(N / 2 - i) > M && A[0][i][j] != 0.0) {
				ok = false;
				std::stringstream ss;
				ss << "FAIL B " << i << "/" << j << "\n";
				inncabs::message(ss.str());
			}
		}
	}
	if (ok && abs(sum - 1) > 0.00001) {
		ok = false;
		std::stringstream ss;
		ss << "FAIL SUM B " << fabs(sum - 1) << "\n";
		inncabs::message(ss.str());
	}
	return ok;
}
