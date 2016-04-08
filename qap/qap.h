#pragma once

#include <cilk/cilk.h>

/*
 Adapted from the Insieme compiler "qap" test case.
 Copyright 2012-2014 University of Innsbruck
 */

// A quadratic matrix, dynamically sized
typedef struct _qmatrix {
	int size;
	int data[];
} qmatrix;

qmatrix* qm_create(int size) {
	qmatrix* res = (qmatrix*)malloc(sizeof(qmatrix) + size * size * sizeof(int));
	res->size = size;
	return res;
}

void qm_del(qmatrix* matrix) {
	free(matrix);
}

// a struct describing a QAP instance
typedef struct _problem {
	int size;		// the size of the problem
	qmatrix* A;		// the weight matrix (size x size)
	qmatrix* B;		// the distance matrix (size x size)
	int optimum;		// the value of the optimal solution
} problem;

void qap_del(problem* problem) {
	qm_del(problem->A);
	qm_del(problem->B);
	free(problem);
}

#define get(M,I,J) M->data[I*M->size+J]
#define getA(P,I,J) get(P->A,I,J)
#define getB(P,I,J) get(P->B,I,J)


// a struct representing a (partial) solution to the problem
typedef struct _solution {
	struct _solution* head;		// the solution is forming a linked list
	int pos;			// the location the current facility is assigned to
} solution;

solution* empty() { return 0; }

void print(solution* solution) {
	if(!solution) return;
	print(solution->head);
	printf("-%d", solution->pos);
}

int solve_rec(const std::launch l, problem* problem, solution* partial, int plant, int used_mask, int cur_cost, std::atomic<int>& best_known) {
	// terminal case
	if(plant >= problem->size) {
		return cur_cost;
	}

	if(cur_cost >= best_known) {
		return best_known;
	}

	// fix current position
	for(int i=0; i<problem->size; i++) {
		// check whether current spot is a free spot
		cilk_spawn [=, &best_known]() {
			if(!(1<<i & used_mask)) {
				// extend solution
				solution tmp = {partial, i};

				// compute additional cost of current assignment
				int new_cost = 0;

				int cur_plant = plant;
				solution* cur = &tmp;
				while(cur) {
					int other_pos = cur->pos;

					// add costs between current pair of plants
					new_cost += getA(problem, plant, cur_plant) * getB(problem, i, other_pos);
					new_cost += getA(problem, cur_plant, plant) * getB(problem, other_pos, i);

					// go to next plant
					cur = cur->head;
					cur_plant--;
				}

				// compute recursive rest
				int cur_best = solve_rec(l, problem, &tmp, plant+1, used_mask | (1<<i), cur_cost + new_cost, best_known);

				// update best known solution
				if(cur_best < best_known) {
					int best;
					//   |--- read best ---|         |--- check ---|    |------- update if cur_best is better ------------|
					do { best = best_known; } while (cur_best < best && best_known.compare_exchange_strong(best, cur_best));
				}
			} 
		}();
	}

	cilk_sync;

	return best_known;
}

int solve(const std::launch l, problem* problem) {
	int res;
	solution* map = empty();
	std::atomic<int> best{ 1 << 30 };
	res = solve_rec(l, problem, map, 0, 0, 0, best);
	return res;
}

problem* qap_load(const char* file) {
	FILE* fp = fopen(file, "r");
	std::stringstream ss;
	ss << "Loading Problem File " << file << " ...\n";

	// get problem size
	int problemSize;
	fscanf(fp, "%d", &problemSize);
	ss << "  - problem size: << " << problemSize << std::endl;

	// create problem instance
	problem* res = (problem*)malloc(sizeof(problem));
	res->size = problemSize;
	res->A = qm_create(problemSize);
	res->B = qm_create(problemSize);

	// load matrix A
	for(int i=0; i<problemSize; i++) {
		for(int j=0; j<problemSize; j++) {
			fscanf(fp, "%d", &getA(res,i,j));
		}
	}

	// load matrix B
	for(int i=0; i<problemSize; i++) {
		for(int j=0; j<problemSize; j++) {
			fscanf(fp, "%d", &getB(res,i,j));
		}
	}

	// load optimum
	fscanf(fp, "%d", &(res->optimum));
	ss << "  - optimum: << " << res->optimum << std::endl;
	inncabs::message(ss.str());

	fclose(fp);
	return res;
}
