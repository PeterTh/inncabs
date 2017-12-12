#pragma once

/*
* This program uses an algorithm that we call `cilksort'.
* The algorithm is essentially mergesort:
*
*   cilksort(in[1..n]) =
*       spawn cilksort(in[1..n/2], tmp[1..n/2])
*       spawn cilksort(in[n/2..n], tmp[n/2..n])
*       sync
*       spawn cilkmerge(tmp[1..n/2], tmp[n/2..n], in[1..n])
*
*
* The procedure cilkmerge does the following:
*
*       cilkmerge(A[1..n], B[1..m], C[1..(n+m)]) =
*          find the median of A \union B using binary
*          search.  The binary search gives a pair
*          (ma, mb) such that ma + mb = (n + m)/2
*          and all elements in A[1..ma] are smaller than
*          B[mb..m], and all the B[1..mb] are smaller
*          than all elements in A[ma..n].
*
*          spawn cilkmerge(A[1..ma], B[1..mb], C[1..(n+m)/2])
*          spawn cilkmerge(A[ma..m], B[mb..n], C[(n+m)/2 .. (n+m)])
*          sync
*
* The algorithm appears for the first time (AFAIK) in S. G. Akl and
* N. Santoro, "Optimal Parallel Merging and Sorting Without Memory
* Conflicts", IEEE Trans. Comp., Vol. C-36 No. 11, Nov. 1987 .  The
* paper does not express the algorithm using recursion, but the
* idea of finding the median is there.
*
* For cilksort of n elements, T_1 = O(n log n) and
* T_\infty = O(log^3 n).  There is a way to shave a
* log factor in the critical path (left as homework).
*/

#include "../include/inncabs.h"

#include "allscale/api/core/prec.h"

#include <cstring>

typedef long ELM;

void seqquick(ELM *low, ELM *high);
void seqmerge(ELM *low1, ELM *high1, ELM *low2, ELM *high2, ELM *lowdest);
ELM *binsplit(ELM val, ELM *low, ELM *high);
void cilkmerge(ELM *low1, ELM *high1, ELM *low2, ELM *high2, ELM *lowdest);
void cilkmerge_par(const std::launch l, ELM *low1, ELM *high1, ELM *low2, ELM *high2, ELM *lowdest);
void cilksort(ELM *low, ELM *tmp, long size);
void cilksort_par(const std::launch l, ELM *low, ELM *tmp, long size);
void scramble_array(ELM *array);
void fill_array(ELM *array);
void sort();

void sort_par(const std::launch l);
void sort_init();
bool sort_verify();

ELM *array, *tmp;
ELM arg_size, arg_cutoff_1, arg_cutoff_2, arg_cutoff_3;

void print(ELM* arr, int n) {
	for(int i=0; i<n; ++i) {
		std::cout << arr[i] << ",";
	}
	std::cout << std::endl;
}

static unsigned long rand_nxt = 0;

static inline unsigned long my_rand(void) {
	rand_nxt = rand_nxt * 1103515245 + 12345;
	return rand_nxt;
}

static inline void my_srand(unsigned long seed) {
	rand_nxt = seed;
}

int cmpfunc(const void* a, const void* b) {
	return *(ELM*)a - *(ELM*)b;
}

void seqmerge(ELM *low1, ELM *high1, ELM *low2, ELM *high2, ELM *lowdest) {
	ELM a1, a2;

	/*
	* The following 'if' statement is not necessary
	* for the correctness of the algorithm, and is
	* in fact subsumed by the rest of the function.
	* However, it is a few percent faster.  Here is why.
	*
	* The merging loop below has something like
	*   if (a1 < a2) {
	*        *dest++ = a1;
	*        ++low1;
	*        if (end of array) break;
	*        a1 = *low1;
	*   }
	*
	* Now, a1 is needed immediately in the next iteration
	* and there is no way to mask the latency of the load.
	* A better approach is to load a1 *before* the end-of-array
	* check; the problem is that we may be speculatively
	* loading an element out of range.  While this is
	* probably not a problem in practice, yet I don't feel
	* comfortable with an incorrect algorithm.  Therefore,
	* I use the 'fast' loop on the array (except for the last
	* element) and the 'slow' loop for the rest, saving both
	* performance and correctness.
	*/

	if(low1 < high1 && low2 < high2) {
		a1 = *low1;
		a2 = *low2;
		while(1) {
			if(a1 < a2) {
				*lowdest++ = a1;
				a1 = *++low1;
				if (low1 >= high1)
					break;
			} else {
				*lowdest++ = a2;
				a2 = *++low2;
				if (low2 >= high2)
					break;
			}
		}
	}
	if(low1 <= high1 && low2 <= high2) {
		a1 = *low1;
		a2 = *low2;
		while(1) {
			if (a1 < a2) {
				*lowdest++ = a1;
				++low1;
				if(low1 > high1)
					break;
				a1 = *low1;
			} else {
				*lowdest++ = a2;
				++low2;
				if (low2 > high2)
					break;
				a2 = *low2;
			}
		}
	}
	if(low1 > high1) {
		memcpy(lowdest, low2, sizeof(ELM) * (high2 - low2 + 1));
	} else {
		memcpy(lowdest, low1, sizeof(ELM) * (high1 - low1 + 1));
	}
}

#define swap_indices(a, b) \
{ \
	ELM *tmp;\
	tmp = a;\
	a = b;\
	b = tmp;\
}

ELM *binsplit(ELM val, ELM *low, ELM *high) {
	/*
	* returns index which contains greatest element <= val.  If val is
	* less than all elements, returns low-1
	*/
	ELM *mid;

	while(low != high) {
		mid = low + ((high - low + 1) >> 1);
		if (val <= *mid)
			high = mid - 1;
		else
			low = mid;
	}

	if(*low > val)
		return low - 1;
	else
		return low;
}




struct merge_params {
	ELM *low1, *high1, *low2, *high2, *lowdest;
};

template<typename MergeFun>
void cilkmerge_par_step(const merge_params& p, const MergeFun& merge) {

	ELM* low1 = p.low1;
	ELM* low2 = p.low2;
	ELM* high1 = p.high1;
	ELM* high2 = p.high2;
	ELM* lowdest = p.lowdest;

	/*
	* Cilkmerge: Merges range [low1, high1] with range [low2, high2]
	* into the range [lowdest, ...]
	*/

	ELM *split1, *split2;	/*
							* where each of the ranges are broken for
							* recursive merge
							*/
	long int lowsize;		/*
							* total size of lower halves of two
							* ranges - 2
							*/

	/*
	* We want to take the middle element (indexed by split1) from the
	* larger of the two arrays.  The following code assumes that split1
	* is taken from range [low1, high1].  So if [low1, high1] is
	* actually the smaller range, we should swap it with [low2, high2]
	*/

	if(high2 - low2 > high1 - low1) {
		swap_indices(low1, low2);
		swap_indices(high1, high2);
	}
//	if(high2 < low2) {
//		/* smaller range is empty */
//		memcpy(lowdest, low1, sizeof(ELM) * (high1 - low1));
//		return;
//	}
//	if(high2 - low2 < arg_cutoff_1 ) {
//		seqmerge(low1, high1, low2, high2, lowdest);
//		return;
//	}

	/*
	* Basic approach: Find the middle element of one range (indexed by
	* split1). Find where this element would fit in the other range
	* (indexed by split 2). Then merge the two lower halves and the two
	* upper halves.
	*/

	split1 = ((high1 - low1 + 1) / 2) + low1;
	split2 = binsplit(*split1, low2, high2);
	lowsize = split1 - low1 + split2 - low2;

	/*
	* directly put the splitting element into
	* the appropriate location
	*/
	*(lowdest + lowsize + 1) = *split1;

    allscale::api::core::parallel(
    	merge({low1, split1 - 1, low2, split2, lowdest}),
	    merge({split1 + 1, high1, split2 + 1, high2, lowdest + lowsize + 2})
    ).get();
}

struct sort_params {
	ELM* low;
	ELM* tmp;
	long size;
};

template<typename SortFun, typename MergeFun>
void cilksort_par_step(const sort_params& p, const SortFun& sort, const MergeFun& merge) {

	ELM* low = p.low;
	ELM* tmp = p.tmp;
	long size = p.size;

	/*
	* divide the input in four parts of the same size (A, B, C, D)
	* Then:
	*   1) recursively sort A, B, C, and D (in parallel)
	*   2) merge A and B into tmp1, and C and D into tmp2 (in parallel)
	*   3) merge tmp1 and tmp2 into the original array
	*/
	long quarter = size / 4;
	ELM *A, *B, *C, *D, *tmpA, *tmpB, *tmpC, *tmpD;

	if(size < arg_cutoff_2) {
		/* quicksort when less than cutoff elements */
		qsort(low, size, sizeof(ELM), &cmpfunc);
		return;
	}
	A = low;
	tmpA = tmp;
	B = A + quarter;
	tmpB = tmpA + quarter;
	C = B + quarter;
	tmpC = tmpB + quarter;
	D = C + quarter;
	tmpD = tmpC + quarter;

    allscale::api::core::sequential(
        allscale::api::core::parallel(
            sort({A, tmpA, quarter}),
            sort({B, tmpB, quarter}),
            sort({C, tmpC, quarter}),
            sort({D, tmpD, size - 3 * quarter})
        ),
        allscale::api::core::parallel(
            merge({A, A + quarter - 1, B, B + quarter - 1, tmpA}),
            merge({C, C + quarter - 1, D, low + size - 1, tmpC})
        ),
    	merge({tmpA, tmpC - 1, tmpC, tmpA + size - 1, A})
    ).get();

}

void cilksort_par(const std::launch l, ELM *low, ELM *tmp, long size) {


	auto rec_qsort = allscale::api::core::prec(
		allscale::api::core::fun(													// quick sort
			[](const sort_params& p) {
				return p.size < arg_cutoff_2;
			},
			[](const sort_params& p) {
				qsort(p.low, p.size, sizeof(ELM), &cmpfunc);
			},
			[](const sort_params& p, const auto& sort, const auto& merge) {
				cilksort_par_step(p,sort,merge);
			}
		),
		allscale::api::core::fun(													// merge step
			[](const merge_params& p) {

				ELM* low1 = p.low1;
				ELM* low2 = p.low2;
				ELM* high1 = p.high1;
				ELM* high2 = p.high2;
				ELM* lowdest = p.lowdest;

				if(high2 - low2 > high1 - low1) {
					swap_indices(low1, low2);
					swap_indices(high1, high2);
				}
				if(high2 < low2) {
					/* smaller range is empty */
					memcpy(lowdest, low1, sizeof(ELM) * (high1 - low1));
					return true;
				}
				if(high2 - low2 < arg_cutoff_1 ) {
					seqmerge(low1, high1, low2, high2, lowdest);
					return true;
				}

				return false;
			},
			[](const merge_params& p) { },
			[](const merge_params& p, const auto&, const auto& merge) {
				cilkmerge_par_step(p,merge);
			}
		)
	);

	// run sorting ...
	rec_qsort({ low, tmp, size}).get();

}

void scramble_array(ELM *array) {
	unsigned long j;

	for (ELM i = 0; i < arg_size; ++i) {
		j = my_rand();
		j = j % arg_size;
		std::swap(array[i], array[j]);
	}
}

void fill_array(ELM *array) {
	my_srand(1);
	/* first, fill with integers 1..size */
	for (ELM i = 0; i < arg_size; ++i) {
		array[i] = i;
	}
}

void sort_init() {
	/* Checking arguments */
	if(arg_size < 4) {
		inncabs::message("N can not be less than 4, using 4 as a parameter.");
		arg_size = 4;
	}

	if(arg_cutoff_1 < 2) {
		inncabs::message("Cutoff 1 can not be less than 2, using 2 as a parameter.\n");
		arg_cutoff_1 = 2;
	}
	else if(arg_cutoff_1 > arg_size ) {
		std::stringstream ss;
		ss << "Cutoff 1 can not be greater than vector size, using " << arg_size << " as a parameter.\n";
		inncabs::message(ss.str());
		arg_cutoff_1 = arg_size;
	}

	if(arg_cutoff_2 > arg_size ) {
		std::stringstream ss;
		ss << "Cutoff 2 can not be greater than vector size, using " << arg_size << " as a parameter.\n";
		inncabs::message(ss.str());
		arg_cutoff_2 = arg_size;
	}
	if(arg_cutoff_3 > arg_size ) {
		std::stringstream ss;
		ss << "Cutoff 3 can not be greater than vector size, using " << arg_size << " as a parameter.\n";
		inncabs::message(ss.str());
		arg_cutoff_3 = arg_size;
	}

	if(arg_cutoff_3 > arg_cutoff_2) {
		std::stringstream ss;
		ss << "Cutoff 3 can not be greater than cutoff 2 , using " << arg_cutoff_2 << " as cutoff 3.\n";
		inncabs::message(ss.str());
		arg_cutoff_3 = arg_cutoff_2;
	}

	array = (ELM *) malloc(arg_size * sizeof(ELM));
	tmp = (ELM *) malloc(arg_size * sizeof(ELM));
	fill_array(array);
	scramble_array(array);
}

void sort_par(const std::launch l) {
	inncabs::message("Computing multisort algorithm");
	cilksort_par(l, array, tmp, arg_size);
	inncabs::message(" completed!\n");
}

bool sort_verify() {
	for(int i = 0; i < arg_size; ++i) {
		if(array[i] != i) {
			return false;
		}
	}
	free(array);
	free(tmp);
	return true;
}
